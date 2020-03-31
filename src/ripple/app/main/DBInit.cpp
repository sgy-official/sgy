

#include <ripple/app/main/DBInit.h>
#include <type_traits>

namespace ripple {

const char* TxnDBName = "transaction.db";
const char* TxnDBInit[] =
{
    "PRAGMA page_size=4096;",
    "PRAGMA synchronous=NORMAL;",
    "PRAGMA journal_mode=WAL;",
    "PRAGMA journal_size_limit=1582080;",
    "PRAGMA max_page_count=2147483646;",

#if (ULONG_MAX > UINT_MAX) && !defined (NO_SQLITE_MMAP)
    "PRAGMA mmap_size=17179869184;",
#endif

    "BEGIN TRANSACTION;",

    "CREATE TABLE IF NOT EXISTS Transactions (                \
        TransID     CHARACTER(64) PRIMARY KEY,  \
        TransType   CHARACTER(24),              \
        FromAcct    CHARACTER(35),              \
        FromSeq     BIGINT UNSIGNED,            \
        LedgerSeq   BIGINT UNSIGNED,            \
        Status      CHARACTER(1),               \
        RawTxn      BLOB,                       \
        TxnMeta     BLOB                        \
    );",
    "CREATE INDEX IF NOT EXISTS TxLgrIndex ON                 \
        Transactions(LedgerSeq);",

    "CREATE TABLE IF NOT EXISTS AccountTransactions (         \
        TransID     CHARACTER(64),              \
        Account     CHARACTER(64),              \
        LedgerSeq   BIGINT UNSIGNED,            \
        TxnSeq      INTEGER                     \
    );",
    "CREATE INDEX IF NOT EXISTS AcctTxIDIndex ON              \
        AccountTransactions(TransID);",
    "CREATE INDEX IF NOT EXISTS AcctTxIndex ON                \
        AccountTransactions(Account, LedgerSeq, TxnSeq, TransID);",
    "CREATE INDEX IF NOT EXISTS AcctLgrIndex ON               \
        AccountTransactions(LedgerSeq, Account, TransID);",

    "END TRANSACTION;"
};

int TxnDBCount = std::extent<decltype(TxnDBInit)>::value;

const char* LedgerDBInit[] =
{
    "PRAGMA synchronous=NORMAL;",
    "PRAGMA journal_mode=WAL;",
    "PRAGMA journal_size_limit=1582080;",

    "BEGIN TRANSACTION;",

    "CREATE TABLE IF NOT EXISTS Ledgers (                         \
        LedgerHash      CHARACTER(64) PRIMARY KEY,  \
        LedgerSeq       BIGINT UNSIGNED,            \
        PrevHash        CHARACTER(64),              \
        TotalCoins      BIGINT UNSIGNED,            \
        ClosingTime     BIGINT UNSIGNED,            \
        PrevClosingTime BIGINT UNSIGNED,            \
        CloseTimeRes    BIGINT UNSIGNED,            \
        CloseFlags      BIGINT UNSIGNED,            \
        AccountSetHash  CHARACTER(64),              \
        TransSetHash    CHARACTER(64)               \
    );",
    "CREATE INDEX IF NOT EXISTS SeqLedger ON Ledgers(LedgerSeq);",

    "CREATE TABLE IF NOT EXISTS Validations   (                   \
        LedgerSeq   BIGINT UNSIGNED,                \
        InitialSeq  BIGINT UNSIGNED,                \
        LedgerHash  CHARACTER(64),                  \
        NodePubKey  CHARACTER(56),                  \
        SignTime    BIGINT UNSIGNED,                \
        RawData     BLOB                            \
    );",
    "CREATE INDEX IF NOT EXISTS ValidationsByHash ON              \
        Validations(LedgerHash);",
    "CREATE INDEX IF NOT EXISTS ValidationsBySeq ON              \
        Validations(LedgerSeq);",
    "CREATE INDEX IF NOT EXISTS ValidationsByInitialSeq ON              \
        Validations(InitialSeq, LedgerSeq);",
    "CREATE INDEX IF NOT EXISTS ValidationsByTime ON              \
        Validations(SignTime);",

    "END TRANSACTION;"
};

int LedgerDBCount = std::extent<decltype(LedgerDBInit)>::value;

const char* WalletDBInit[] =
{
    "BEGIN TRANSACTION;",

    "CREATE TABLE IF NOT EXISTS NodeIdentity (      \
        PublicKey       CHARACTER(53),              \
        PrivateKey      CHARACTER(52)               \
    );",

    "CREATE TABLE IF NOT EXISTS ValidatorManifests ( \
        RawData          BLOB NOT NULL               \
    );",

    "CREATE TABLE IF NOT EXISTS PublisherManifests ( \
        RawData          BLOB NOT NULL               \
    );",

    "DROP INDEX IF EXISTS SeedNodeNext;",
    "DROP INDEX IF EXISTS SeedDomainNext;",
    "DROP TABLE IF EXISTS Features;",
    "DROP TABLE IF EXISTS TrustedNodes;",
    "DROP TABLE IF EXISTS ValidatorReferrals;",
    "DROP TABLE IF EXISTS IpReferrals;",
    "DROP TABLE IF EXISTS SeedNodes;",
    "DROP TABLE IF EXISTS SeedDomains;",
    "DROP TABLE IF EXISTS Misc;",

    "END TRANSACTION;"
};

int WalletDBCount = std::extent<decltype(WalletDBInit)>::value;

} 
























