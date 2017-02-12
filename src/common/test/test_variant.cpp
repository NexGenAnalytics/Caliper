#include "../Variant.h"

#include "gtest/gtest.h"

#include <cstring>

using namespace cali;

// Test some type conversions

TEST(Variant_Test, FromString) {
    const char* teststr = "My wonderful test string";
    uint64_t uval = 0xef10;
    
    const struct testcase_t {
        cali_attr_type type;
        const char*    str;
        bool           ok;
        Variant        expected;
    } testcases[] = {
        { CALI_TYPE_INV,    "42",              false, Variant() },

        { CALI_TYPE_INT,    "42",              true,  Variant(static_cast<int>(42))  },
        { CALI_TYPE_INT,    " -10 ",           true,  Variant(static_cast<int>(-10)) },
        { CALI_TYPE_INT,    "bla",             false, Variant() },
        
        { CALI_TYPE_STRING, teststr,           true,  Variant(CALI_TYPE_STRING, teststr, strlen(teststr)) },
        { CALI_TYPE_STRING, "",                true,  Variant(CALI_TYPE_STRING, "", 0)  },

        { CALI_TYPE_UINT,   "0",               true,  Variant(static_cast<unsigned>(0)) },
        { CALI_TYPE_UINT,   "1337",            true,  Variant(static_cast<unsigned>(1337)) },
        { CALI_TYPE_ADDR,   "ef10",            true,  Variant(CALI_TYPE_ADDR, &uval, sizeof(uval))   },

        { CALI_TYPE_DOUBLE, "-1.0",            true,  Variant(static_cast<double>(-1.0)) },

        { CALI_TYPE_BOOL,   "false",           true,  Variant(false) },
        { CALI_TYPE_BOOL,   "1",               true,  Variant(true)  },
        { CALI_TYPE_BOOL,   "bla",             false, Variant()      },

        { CALI_TYPE_TYPE,   "int",             true,  Variant(CALI_TYPE_INT) },
        { CALI_TYPE_TYPE,   "bla",             false, Variant()      },

        { CALI_TYPE_INV, 0, false, Variant() }
    };

    for (const testcase_t* t = testcases; t->str; ++t) {
        bool    ok = false;
        Variant v(Variant::from_string(t->type, t->str, &ok));
        
        EXPECT_EQ(ok, t->ok) << "for \"" << t->str << "\" (" << cali_type2string(t->type) << ")";
        EXPECT_EQ(v, t->expected) << "for \"" << t->str << "\" (" << cali_type2string(t->type) << ")";
    }
}

// --- Test Variant pack/unpack

TEST(Variant_Test, PackUnpack) {
    int            val_1_int  = -27;
    uint64_t       val_2_uint = 0xFFFFFFFFAA;
    const char*    val_3_str  = "My wonderful test string";
    double         val_4_dbl  = 42.42;
    const void*    val_5_inv  = NULL;
    cali_attr_type val_6_type = CALI_TYPE_ADDR;
    bool           val_7_bool = true;

    cali::Variant  v_1_int_in(val_1_int);
    cali::Variant  v_2_uint_in(CALI_TYPE_UINT, &val_2_uint, sizeof(uint64_t));
    cali::Variant  v_3_str_in(CALI_TYPE_STRING, val_3_str, strlen(val_3_str)+1);
    cali::Variant  v_4_dbl_in(val_4_dbl);
    cali::Variant  v_5_inv_in;
    cali::Variant  v_6_type_in(val_6_type);
    cali::Variant  v_7_bool_in(val_7_bool);

    unsigned char buf[160]; // must be >= 7*22 = 154 bytes
    size_t pos = 0;

    memset(buf, 0, 160);

    pos += v_1_int_in.pack(buf+pos);
    pos += v_2_uint_in.pack(buf+pos);
    pos += v_3_str_in.pack(buf+pos);
    pos += v_4_dbl_in.pack(buf+pos);
    pos += v_5_inv_in.pack(buf+pos);
    pos += v_6_type_in.pack(buf+pos);
    pos += v_7_bool_in.pack(buf+pos);

    EXPECT_LE(pos, 154);

    bool ok = false;
    pos = 0;

    Variant v_1_int_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_1 unpack (int)");
    Variant v_2_uint_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_2 unpack (uint)");
    Variant v_3_str_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_3 unpack (string)");
    Variant v_4_dbl_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_4 unpack (double)");
    Variant v_5_inv_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_5 unpack (inv)");
    Variant v_6_type_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_6 unpack (type)");
    Variant v_7_bool_out = Variant::unpack(buf+pos, &pos, &ok);
    EXPECT_TRUE(ok && "v_7 unpack (bool)");

    EXPECT_EQ(v_1_int_out.type(), CALI_TYPE_INT);
    EXPECT_EQ(v_1_int_out.to_int(), val_1_int);

    EXPECT_EQ(v_2_uint_out.type(), CALI_TYPE_UINT);
    EXPECT_EQ(v_2_uint_out.to_uint(), val_2_uint);

    EXPECT_EQ(v_3_str_out.type(), CALI_TYPE_STRING);
    EXPECT_EQ(v_3_str_out.to_string(), std::string(val_3_str));

    EXPECT_EQ(v_4_dbl_out.type(), CALI_TYPE_DOUBLE);
    EXPECT_EQ(v_4_dbl_out.to_double(), val_4_dbl);

    EXPECT_EQ(v_5_inv_out.type(), CALI_TYPE_INV);
    EXPECT_TRUE(v_5_inv_out.empty());

    EXPECT_EQ(v_6_type_out.type(), CALI_TYPE_TYPE);
    EXPECT_EQ(v_6_type_out.to_attr_type(), val_6_type);

    EXPECT_EQ(v_7_bool_out.type(), CALI_TYPE_BOOL);
    EXPECT_EQ(v_7_bool_out.to_bool(), val_7_bool);
}
