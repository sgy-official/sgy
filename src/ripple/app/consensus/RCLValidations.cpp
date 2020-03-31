

#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/chrono.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/TimeKeeper.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

RCLValidatedLedger::RCLValidatedLedger(MakeGenesis)
    : ledgerID_{0}, ledgerSeq_{0}, j_{beast::Journal::getNullSink()}
{
}

RCLValidatedLedger::RCLValidatedLedger(
    std::shared_ptr<Ledger const> const& ledger,
    beast::Journal j)
    : ledgerID_{ledger->info().hash}, ledgerSeq_{ledger->seq()}, j_{j}
{
    auto const hashIndex = ledger->read(keylet::skip());
    if (hashIndex)
    {
        assert(hashIndex->getFieldU32(sfLastLedgerSequence) == (seq() - 1));
        ancestors_ = hashIndex->getFieldV256(sfHashes).value();
    }
    else
        JLOG(j_.warn()) << "Ledger " << ledgerSeq_ << ":" << ledgerID_
                        << " missing recent ancestor hashes";
}

auto
RCLValidatedLedger::minSeq() const -> Seq
{
    return seq() - std::min(seq(), static_cast<Seq>(ancestors_.size()));
}

auto
RCLValidatedLedger::seq() const -> Seq
{
    return ledgerSeq_;
}
auto
RCLValidatedLedger::id() const -> ID
{
    return ledgerID_;
}

auto RCLValidatedLedger::operator[](Seq const& s) const -> ID
{
    if (s >= minSeq() && s <= seq())
    {
        if (s == seq())
            return ledgerID_;
        Seq const diff = seq() - s;
        return ancestors_[ancestors_.size() - diff];
    }

    JLOG(j_.warn()) << "Unable to determine hash of ancestor seq=" << s
                    << " from ledger hash=" << ledgerID_
                    << " seq=" << ledgerSeq_;
    return ID{0};
}

RCLValidatedLedger::Seq
mismatch(RCLValidatedLedger const& a, RCLValidatedLedger const& b)
{
    using Seq = RCLValidatedLedger::Seq;

    Seq const lower = std::max(a.minSeq(), b.minSeq());
    Seq const upper = std::min(a.seq(), b.seq());

    Seq curr = upper;
    while (curr != Seq{0} && a[curr] != b[curr] && curr >= lower)
        --curr;

    return (curr < lower) ? Seq{1} : (curr + Seq{1});
}

RCLValidationsAdaptor::RCLValidationsAdaptor(Application& app, beast::Journal j)
    : app_(app),  j_(j)
{
    staleValidations_.reserve(512);
}

NetClock::time_point
RCLValidationsAdaptor::now() const
{
    return app_.timeKeeper().closeTime();
}

boost::optional<RCLValidatedLedger>
RCLValidationsAdaptor::acquire(LedgerHash const & hash)
{
    auto ledger = app_.getLedgerMaster().getLedgerByHash(hash);
    if (!ledger)
    {
        JLOG(j_.debug())
            << "Need validated ledger for preferred ledger analysis " << hash;

        Application * pApp = &app_;

        app_.getJobQueue().addJob(
            jtADVANCE, "getConsensusLedger", [pApp, hash](Job&) {
                pApp ->getInboundLedgers().acquire(
                    hash, 0, InboundLedger::Reason::CONSENSUS);
            });
        return boost::none;
    }

    assert(!ledger->open() && ledger->isImmutable());
    assert(ledger->info().hash == hash);

    return RCLValidatedLedger(std::move(ledger), j_);
}

void
RCLValidationsAdaptor::onStale(RCLValidation&& v)
{

    ScopedLockType sl(staleLock_);
    staleValidations_.emplace_back(std::move(v));
    if (staleWriting_)
        return;

    staleWriting_ = app_.getJobQueue().addJob(
        jtWRITE, "Validations::doStaleWrite", [this](Job&) {
            auto event =
                app_.getJobQueue().makeLoadEvent(jtDISK, "ValidationWrite");
            ScopedLockType sl(staleLock_);
            doStaleWrite(sl);
        });
}

void
RCLValidationsAdaptor::flush(hash_map<NodeID, RCLValidation>&& remaining)
{
    bool anyNew = false;
    {
        ScopedLockType sl(staleLock_);

        for (auto const& keyVal : remaining)
        {
            staleValidations_.emplace_back(std::move(keyVal.second));
            anyNew = true;
        }

        if (anyNew && !staleWriting_)
        {
            staleWriting_ = true;
            doStaleWrite(sl);
        }


        while (staleWriting_)
        {
            ScopedUnlockType sul(staleLock_);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void
RCLValidationsAdaptor::doStaleWrite(ScopedLockType&)
{
    static const std::string insVal(
        "INSERT INTO Validations "
        "(InitialSeq, LedgerSeq, LedgerHash,NodePubKey,SignTime,RawData) "
        "VALUES (:initialSeq, :ledgerSeq, "
        ":ledgerHash,:nodePubKey,:signTime,:rawData);");
    static const std::string findSeq(
        "SELECT LedgerSeq FROM Ledgers WHERE Ledgerhash=:ledgerHash;");

    assert(staleWriting_);

    while (!staleValidations_.empty())
    {
        std::vector<RCLValidation> currentStale;
        currentStale.reserve(512);
        staleValidations_.swap(currentStale);

        {
            ScopedUnlockType sul(staleLock_);
            {
                auto db = app_.getLedgerDB().checkoutDb();

                Serializer s(1024);
                soci::transaction tr(*db);
                for (RCLValidation const& wValidation : currentStale)
                {
                    if(!wValidation.full())
                        continue;
                    s.erase();
                    STValidation::pointer const& val = wValidation.unwrap();
                    val->add(s);

                    auto const ledgerHash = to_string(val->getLedgerHash());

                    boost::optional<std::uint64_t> ledgerSeq;
                    *db << findSeq, soci::use(ledgerHash),
                        soci::into(ledgerSeq);

                    auto const initialSeq = ledgerSeq.value_or(
                        app_.getLedgerMaster().getCurrentLedgerIndex());
                    auto const nodePubKey = toBase58(
                        TokenType::NodePublic, val->getSignerPublic());
                    auto const signTime =
                        val->getSignTime().time_since_epoch().count();

                    soci::blob rawData(*db);
                    rawData.append(
                        reinterpret_cast<const char*>(s.peekData().data()),
                        s.peekData().size());
                    assert(rawData.get_len() == s.peekData().size());

                    *db << insVal, soci::use(initialSeq), soci::use(ledgerSeq),
                        soci::use(ledgerHash), soci::use(nodePubKey),
                        soci::use(signTime), soci::use(rawData);
                }

                tr.commit();
            }
        }
    }

    staleWriting_ = false;
}

bool
handleNewValidation(Application& app,
    STValidation::ref val,
    std::string const& source)
{
    PublicKey const& signingKey = val->getSignerPublic();
    uint256 const& hash = val->getLedgerHash();

    boost::optional<PublicKey> masterKey =
        app.validators().getTrustedKey(signingKey);
    if (!val->isTrusted() && masterKey)
        val->setTrusted();

    if (!masterKey)
        masterKey = app.validators().getListedKey(signingKey);

    bool shouldRelay = false;
    RCLValidations& validations = app.getValidations();
    beast::Journal j = validations.adaptor().journal();

    auto dmp = [&](beast::Journal::Stream s, std::string const& msg) {
        s << "Val for " << hash
          << (val->isTrusted() ? " trusted/" : " UNtrusted/")
          << (val->isFull() ? "full" : "partial") << " from "
          << (masterKey ? toBase58(TokenType::NodePublic, *masterKey)
                        : "unknown")
          << " signing key "
          << toBase58(TokenType::NodePublic, signingKey) << " " << msg
          << " src=" << source;
    };

    if(!val->isFieldPresent(sfLedgerSequence))
    {
        if(j.error())
            dmp(j.error(), "missing ledger sequence field");
        return false;
    }

    if (masterKey)
    {
        ValStatus const outcome = validations.add(calcNodeID(*masterKey), val);
        if(j.debug())
            dmp(j.debug(), to_string(outcome));

        if (outcome == ValStatus::badSeq && j.warn())
        {
            auto const seq = val->getFieldU32(sfLedgerSequence);
            dmp(j.warn(),
                "already validated sequence at or past " + to_string(seq));
        }

        if (val->isTrusted() && outcome == ValStatus::current)
        {
            app.getLedgerMaster().checkAccept(
                hash, val->getFieldU32(sfLedgerSequence));
            shouldRelay = true;
        }
    }
    else
    {
        JLOG(j.debug()) << "Val for " << hash << " from "
                    << toBase58(TokenType::NodePublic, signingKey)
                    << " not added UNlisted";
    }

    return shouldRelay;
}


}  
























