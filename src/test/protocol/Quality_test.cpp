

#include <ripple/protocol/Quality.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {

class Quality_test : public beast::unit_test::suite
{
public:
    STAmount
    static raw (std::uint64_t mantissa, int exponent)
    {
        return STAmount ({Currency(3), AccountID(3)}, mantissa, exponent);
    }

    template <class Integer>
    static
    STAmount
    amount (Integer integer,
        std::enable_if_t <std::is_signed <Integer>::value>* = 0)
    {
        static_assert (std::is_integral <Integer>::value, "");
        return STAmount (integer, false);
    }

    template <class Integer>
    static
    STAmount
    amount (Integer integer,
        std::enable_if_t <! std::is_signed <Integer>::value>* = 0)
    {
        static_assert (std::is_integral <Integer>::value, "");
        if (integer < 0)
            return STAmount (-integer, true);
        return STAmount (integer, false);
    }

    template <class In, class Out>
    static
    Amounts
    amounts (In in, Out out)
    {
        return Amounts (amount(in), amount(out));
    }

    template <class In1, class Out1, class Int, class In2, class Out2>
    void
    ceil_in (Quality const& q,
        In1 in, Out1 out, Int limit, In2 in_expected, Out2 out_expected)
    {
        auto expect_result (amounts (in_expected, out_expected));
        auto actual_result (q.ceil_in (
            amounts (in, out), amount (limit)));

        BEAST_EXPECT(actual_result == expect_result);
    }

    template <class In1, class Out1, class Int, class In2, class Out2>
    void
    ceil_out (Quality const& q,
        In1 in, Out1 out, Int limit, In2 in_expected, Out2 out_expected)
    {
        auto const expect_result (amounts (in_expected, out_expected));
        auto const actual_result (q.ceil_out (
            amounts (in, out), amount (limit)));

        BEAST_EXPECT(actual_result == expect_result);
    }

    void
    test_ceil_in ()
    {
        testcase ("ceil_in");

        {
            Quality q (Amounts (amount(1), amount(1)));

            ceil_in (q,
                1,  1,   
                1,       
                1,  1);  

            ceil_in (q,
                10, 10, 
                5,      
                5, 5);  

            ceil_in (q,
                5, 5,   
                10,     
                5, 5);  
        }

        {
            Quality q (Amounts (amount(1), amount(2)));

            ceil_in (q,
                40, 80,   
                40,       
                40, 80);  

            ceil_in (q,
                40, 80,   
                20,       
                20, 40);  

            ceil_in (q,
                40, 80,   
                60,       
                40, 80);  
        }

        {
            Quality q (Amounts (amount(2), amount(1)));

            ceil_in (q,
                40, 20,   
                20,       
                20, 10);  

            ceil_in (q,
                40, 20,   
                40,       
                40, 20);  

            ceil_in (q,
                40, 20,   
                50,       
                40, 20);  
        }
    }

    void
    test_ceil_out ()
    {
        testcase ("ceil_out");

        {
            Quality q (Amounts (amount(1),amount(1)));

            ceil_out (q,
                1,  1,    
                1,        
                1,  1);   

            ceil_out (q,
                10, 10,   
                5,        
                5, 5);    

            ceil_out (q,
                10, 10,   
                20,       
                10, 10);  
        }

        {
            Quality q (Amounts (amount(1),amount(2)));

            ceil_out (q,
                40, 80,    
                40,        
                20, 40);   

            ceil_out (q,
                40, 80,    
                80,        
                40, 80);   

            ceil_out (q,
                40, 80,    
                100,       
                40, 80);   
        }

        {
            Quality q (Amounts (amount(2),amount(1)));

            ceil_out (q,
                40, 20,    
                20,        
                40, 20);   

            ceil_out (q,
                40, 20,    
                40,        
                40, 20);   

            ceil_out (q,
                40, 20,    
                10,        
                20, 10);   
        }
    }

    void
    test_raw()
    {
        testcase ("raw");

        {
            Quality q (0x5d048191fb9130daull);      
            Amounts const value (
                amount(349469768),                  
                raw (2755280000000000ull, -15));    
            STAmount const limit (
                raw (4131113916555555, -16));       
            Amounts const result (
                q.ceil_out (value, limit));
            BEAST_EXPECT(result.in != beast::zero);
        }
    }

    void
    test_round()
    {
        testcase ("round");

        Quality q (0x59148191fb913522ull);      
        BEAST_EXPECT(q.round(3).rate().getText() == "57800");
        BEAST_EXPECT(q.round(4).rate().getText() == "57720");
        BEAST_EXPECT(q.round(5).rate().getText() == "57720");
        BEAST_EXPECT(q.round(6).rate().getText() == "57719.7");
        BEAST_EXPECT(q.round(7).rate().getText() == "57719.64");
        BEAST_EXPECT(q.round(8).rate().getText() == "57719.636");
        BEAST_EXPECT(q.round(9).rate().getText() == "57719.6353");
        BEAST_EXPECT(q.round(10).rate().getText() == "57719.63526");
        BEAST_EXPECT(q.round(11).rate().getText() == "57719.635251");
        BEAST_EXPECT(q.round(12).rate().getText() == "57719.6352506");
        BEAST_EXPECT(q.round(13).rate().getText() == "57719.63525052");
        BEAST_EXPECT(q.round(14).rate().getText() == "57719.635250517");
        BEAST_EXPECT(q.round(15).rate().getText() == "57719.6352505169");
        BEAST_EXPECT(q.round(16).rate().getText() == "57719.63525051682");
    }

    void
    test_comparisons()
    {
        testcase ("comparisons");

        STAmount const amount1 (noIssue(), 231);
        STAmount const amount2 (noIssue(), 462);
        STAmount const amount3 (noIssue(), 924);

        Quality const q11 (Amounts (amount1, amount1));
        Quality const q12 (Amounts (amount1, amount2));
        Quality const q13 (Amounts (amount1, amount3));
        Quality const q21 (Amounts (amount2, amount1));
        Quality const q31 (Amounts (amount3, amount1));

        BEAST_EXPECT(q11 == q11);
        BEAST_EXPECT(q11 < q12);
        BEAST_EXPECT(q12 < q13);
        BEAST_EXPECT(q31 < q21);
        BEAST_EXPECT(q21 < q11);
        BEAST_EXPECT(q11 >= q11);
        BEAST_EXPECT(q12 >= q11);
        BEAST_EXPECT(q13 >= q12);
        BEAST_EXPECT(q21 >= q31);
        BEAST_EXPECT(q11 >= q21);
        BEAST_EXPECT(q12 > q11);
        BEAST_EXPECT(q13 > q12);
        BEAST_EXPECT(q21 > q31);
        BEAST_EXPECT(q11 > q21);
        BEAST_EXPECT(q11 <= q11);
        BEAST_EXPECT(q11 <= q12);
        BEAST_EXPECT(q12 <= q13);
        BEAST_EXPECT(q31 <= q21);
        BEAST_EXPECT(q21 <= q11);
        BEAST_EXPECT(q31 != q21);
    }

    void
    test_composition ()
    {
        testcase ("composition");

        STAmount const amount1 (noIssue(), 231);
        STAmount const amount2 (noIssue(), 462);
        STAmount const amount3 (noIssue(), 924);

        Quality const q11 (Amounts (amount1, amount1));
        Quality const q12 (Amounts (amount1, amount2));
        Quality const q13 (Amounts (amount1, amount3));
        Quality const q21 (Amounts (amount2, amount1));
        Quality const q31 (Amounts (amount3, amount1));

        BEAST_EXPECT(
            composed_quality (q12, q21) == q11);

        Quality const q13_31 (
            composed_quality (q13, q31));
        Quality const q31_13 (
            composed_quality (q31, q13));

        BEAST_EXPECT(q13_31 == q31_13);
        BEAST_EXPECT(q13_31 == q11);
    }

    void
    test_operations ()
    {
        testcase ("operations");

        Quality const q11 (Amounts (
            STAmount (noIssue(), 731),
            STAmount (noIssue(), 731)));

        Quality qa (q11);
        Quality qb (q11);

        BEAST_EXPECT(qa == qb);
        BEAST_EXPECT(++qa != q11);
        BEAST_EXPECT(qa != qb);
        BEAST_EXPECT(--qb != q11);
        BEAST_EXPECT(qa != qb);
        BEAST_EXPECT(qb < qa);
        BEAST_EXPECT(qb++ < qa);
        BEAST_EXPECT(qb++ < qa);
        BEAST_EXPECT(qb++ == qa);
        BEAST_EXPECT(qa < qb);
    }
    void
    run() override
    {
        test_comparisons ();
        test_composition ();
        test_operations ();
        test_ceil_in ();
        test_ceil_out ();
        test_raw ();
        test_round ();
    }
};

BEAST_DEFINE_TESTSUITE(Quality,protocol,ripple);

}
























