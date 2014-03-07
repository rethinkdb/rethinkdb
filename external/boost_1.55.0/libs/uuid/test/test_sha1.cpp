//  (C) Copyright Andy Tompkins 2007. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_sha1.cpp  -------------------------------//

#include <boost/uuid/sha1.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>
#include <cstring>
#include <cstddef>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
  using ::strlen;
  using ::size_t;
} //namespace std
#endif

void test_sha1_digest_equal_array(char const * file, int line, char const * function,
                          const unsigned int (&lhs)[5], const unsigned int (&rhs)[5])
{
    for (size_t i=0; i<5; i++) {
        if ( lhs[i] != rhs[i]) {
            std::cerr << file << "(" << line << "): sha1 digest [";
            for (size_t l=0; l<5; l++) {
                if (l != 0) {
                    std::cerr << " ";
                }
                std::cerr << std::hex << (int)lhs[l];
            }

            std::cerr << "] not equal [";
            for (size_t r=0; r<5; r++) {
                if (r != 0) {
                    std::cerr << " ";
                }
                std::cerr << std::hex << (int)rhs[r];
            }
            std::cerr << "] in function '" << function << "'" << std::endl;
            ++boost::detail::test_errors();
            return;
        }
    }
}


#define BOOST_TEST_SHA1_DIGEST(lhs, rhs) ( test_sha1_digest_equal_array(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, lhs, rhs) )

void test_sha1(char const*const message, unsigned int length, const unsigned int (&correct_digest)[5])
{
    boost::uuids::detail::sha1 sha;
    sha.process_bytes(message, length);

    unsigned int digest[5];
    sha.get_digest(digest);

    BOOST_TEST_SHA1_DIGEST(digest, correct_digest);
}

void test_quick()
{
    struct test_case
    {
        char const* message;
        unsigned int digest[5];
    };
    test_case cases[] =
    { { "",
        { 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709 } }
    , { "The quick brown fox jumps over the lazy dog",
        { 0x2fd4e1c6, 0x7a2d28fc, 0xed849ee1, 0xbb76e739, 0x1b93eb12 } }
    , { "The quick brown fox jumps over the lazy cog",
        { 0xde9f2c7f, 0xd25e1b3a, 0xfad3e85a, 0x0bd17d9b, 0x100db4b3 } }
    };

    for (int i=0; i!=sizeof(cases)/sizeof(cases[0]); ++i) {
        test_case const& tc = cases[i];
        test_sha1(tc.message, std::strlen(tc.message), tc.digest);
    }
}

//SHA Test Vector for Hashing Byte-Oriented Messages
//http://csrc.nist.gov/cryptval/shs.htm
//http://csrc.nist.gov/cryptval/shs/shabytetestvectors.zip
//values from SHA1ShortMsg.txt
void test_short_messages()
{
    struct test_case
    {
        char const* message;
        unsigned int digest[5];
    };
    test_case cases[] =
    { { "",
         { 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709 } }
    , { "a8", 
         { 0x99f2aa95, 0xe36f95c2, 0xacb0eaf2, 0x3998f030, 0x638f3f15 } }
    , { "3000", 
         { 0xf944dcd6, 0x35f9801f, 0x7ac90a40, 0x7fbc4799, 0x64dec024 } }
    , { "42749e", 
         { 0xa444319e, 0x9b6cc1e8, 0x464c511e, 0xc0969c37, 0xd6bb2619 } }
    , { "9fc3fe08", 
         { 0x16a0ff84, 0xfcc156fd, 0x5d3ca3a7, 0x44f20a23, 0x2d172253 } }
    , { "b5c1c6f1af", 
         { 0xfec9deeb, 0xfcdedaf6, 0x6dda525e, 0x1be43597, 0xa73a1f93 } }
    , { "e47571e5022e", 
         { 0x8ce05118, 0x1f0ed5e9, 0xd0c498f6, 0xbc4caf44, 0x8d20deb5 } }
    , { "3e1b28839fb758", 
         { 0x67da5383, 0x7d89e03b, 0xf652ef09, 0xc369a341, 0x5937cfd3 } }
    , { "a81350cbb224cb90", 
         { 0x305e4ff9, 0x888ad855, 0xa78573cd, 0xdf4c5640, 0xcce7e946 } }
    , { "c243d167923dec3ce1", 
         { 0x5902b77b, 0x3265f023, 0xf9bbc396, 0xba1a93fa, 0x3509bde7 } }
    , { "50ac18c59d6a37a29bf4", 
         { 0xfcade5f5, 0xd156bf6f, 0x9af97bdf, 0xa9c19bcc, 0xfb4ff6ab } }
    , { "98e2b611ad3b1cccf634f6", 
         { 0x1d20fbe0, 0x0533c10e, 0x3cbd6b27, 0x088a5de0, 0xc632c4b5 } }
    , { "73fe9afb68e1e8712e5d4eec", 
         { 0x7e1b7e0f, 0x7a8f3455, 0xa9c03e95, 0x80fd63ae, 0x205a2d93 } }
    , { "9e701ed7d412a9226a2a130e66", 
         { 0x706f0677, 0x146307b2, 0x0bb0e8d6, 0x311e3299, 0x66884d13 } }
    , { "6d3ee90413b0a7cbf69e5e6144ca", 
         { 0xa7241a70, 0x3aaf0d53, 0xfe142f86, 0xbf2e8492, 0x51fa8dff } }
    , { "fae24d56514efcb530fd4802f5e71f", 
         { 0x400f5354, 0x6916d33a, 0xd01a5e6d, 0xf66822df, 0xbdc4e9e6 } }
    , { "c5a22dd6eda3fe2bdc4ddb3ce6b35fd1", 
         { 0xfac8ab93, 0xc1ae6c16, 0xf0311872, 0xb984f729, 0xdc928ccd } }
    , { "d98cded2adabf08fda356445c781802d95", 
         { 0xfba6d750, 0xc18da58f, 0x6e2aab10, 0x112b9a5e, 0xf3301b3b } }
    , { "bcc6d7087a84f00103ccb32e5f5487a751a2", 
         { 0x29d27c2d, 0x44c205c8, 0x107f0351, 0xb05753ac, 0x708226b6 } }
    , { "36ecacb1055434190dbbc556c48bafcb0feb0d", 
         { 0xb971bfc1, 0xebd6f359, 0xe8d74cb7, 0xecfe7f89, 0x8d0ba845 } }
    , { "5ff9edb69e8f6bbd498eb4537580b7fba7ad31d0", 
         { 0x96d08c43, 0x0094b9fc, 0xc164ad2f, 0xb6f72d0a, 0x24268f68 } }
    , { "c95b441d8270822a46a798fae5defcf7b26abace36", 
         { 0xa287ea75, 0x2a593d52, 0x09e28788, 0x1a09c49f, 0xa3f0beb1 } }
    , { "83104c1d8a55b28f906f1b72cb53f68cbb097b44f860", 
         { 0xa06c7137, 0x79cbd885, 0x19ed4a58, 0x5ac0cb8a, 0x5e9d612b } }
    , { "755175528d55c39c56493d697b790f099a5ce741f7754b", 
         { 0xbff7d52c, 0x13a36881, 0x32a1d407, 0xb1ab40f5, 0xb5ace298 } }
    , { "088fc38128bbdb9fd7d65228b3184b3faac6c8715f07272f", 
         { 0xc7566b91, 0xd7b6f56b, 0xdfcaa978, 0x1a7b6841, 0xaacb17e9 } }
    , { "a4a586eb9245a6c87e3adf1009ac8a49f46c07e14185016895", 
         { 0xffa30c0b, 0x5c550ea4, 0xb1e34f8a, 0x60ec9295, 0xa1e06ac1 } }
    , { "8e7c555270c006092c2a3189e2a526b873e2e269f0fb28245256", 
         { 0x29e66ed2, 0x3e914351, 0xe872aa76, 0x1df6e4f1, 0xa07f4b81 } }
    , { "a5f3bfa6bb0ba3b59f6b9cbdef8a558ec565e8aa3121f405e7f2f0", 
         { 0xb28cf5e5, 0xb806a014, 0x91d41f69, 0xbd924876, 0x5c5dc292 } }
    , { "589054f0d2bd3c2c85b466bfd8ce18e6ec3e0b87d944cd093ba36469", 
         { 0x60224fb7, 0x2c460696, 0x52cd78bc, 0xd08029ef, 0x64da62f3 } }
    , { "a0abb12083b5bbc78128601bf1cbdbc0fdf4b862b24d899953d8da0ff3", 
         { 0xb72c4a86, 0xf72608f2, 0x4c05f3b9, 0x088ef92f, 0xba431df7 } }
    , { "82143f4cea6fadbf998e128a8811dc75301cf1db4f079501ea568da68eeb", 
         { 0x73779ad5, 0xd6b71b9b, 0x8328ef72, 0x20ff12eb, 0x167076ac } }
    , { "9f1231dd6df1ff7bc0b0d4f989d048672683ce35d956d2f57913046267e6f3", 
         { 0xa09671d4, 0x452d7cf5, 0x0015c914, 0xa1e31973, 0xd20cc1a0 } }
    , { "041c512b5eed791f80d3282f3a28df263bb1df95e1239a7650e5670fc2187919", 
         { 0xe88cdcd2, 0x33d99184, 0xa6fd260b, 0x8fca1b7f, 0x7687aee0 } }
    , { "17e81f6ae8c2e5579d69dafa6e070e7111461552d314b691e7a3e7a4feb3fae418", 
         { 0x010def22, 0x850deb11, 0x68d525e8, 0xc84c2811, 0x6cb8a269 } }
    , { "d15976b23a1d712ad28fad04d805f572026b54dd64961fda94d5355a0cc98620cf77", 
         { 0xaeaa40ba, 0x1717ed54, 0x39b1e6ea, 0x901b294b, 0xa500f9ad } }
    , { "09fce4d434f6bd32a44e04b848ff50ec9f642a8a85b37a264dc73f130f22838443328f", 
         { 0xc6433791, 0x238795e3, 0x4f080a5f, 0x1f1723f0, 0x65463ca0 } }
    , { "f17af27d776ec82a257d8d46d2b46b639462c56984cc1be9c1222eadb8b26594a25c709d", 
         { 0xe21e22b8, 0x9c1bb944, 0xa32932e6, 0xb2a2f20d, 0x491982c3 } }
    , { "b13ce635d6f8758143ffb114f2f601cb20b6276951416a2f94fbf4ad081779d79f4f195b22", 
         { 0x575323a9, 0x661f5d28, 0x387964d2, 0xba6ab92c, 0x17d05a8a } }
    , { "5498793f60916ff1c918dde572cdea76da8629ba4ead6d065de3dfb48de94d234cc1c5002910", 
         { 0xfeb44494, 0xaf72f245, 0xbfe68e86, 0xc4d7986d, 0x57c11db7 } }
    , { "498a1e0b39fa49582ae688cd715c86fbaf8a81b8b11b4d1594c49c902d197c8ba8a621fd6e3be5", 
         { 0xcff2290b, 0x3648ba28, 0x31b98dde, 0x436a72f9, 0xebf51eee } }
    , { "3a36ae71521f9af628b3e34dcb0d4513f84c78ee49f10416a98857150b8b15cb5c83afb4b570376e", 
         { 0x9b4efe9d, 0x27b96590, 0x5b0c3dab, 0x67b8d7c9, 0xebacd56c } }
    , { "dcc76b40ae0ea3ba253e92ac50fcde791662c5b6c948538cffc2d95e9de99cac34dfca38910db2678f", 
         { 0xafedb0ff, 0x156205bc, 0xd831cbdb, 0xda43db8b, 0x0588c113 } }
    , { "5b5ec6ec4fd3ad9c4906f65c747fd4233c11a1736b6b228b92e90cddabb0c7c2fcf9716d3fad261dff33", 
         { 0x8deb1e85, 0x8f88293a, 0x5e5e4d52, 0x1a34b2a4, 0xefa70fc4 } }
    , { "df48a37b29b1d6de4e94717d60cdb4293fcf170bba388bddf7a9035a15d433f20fd697c3e4c8b8c5f590ab", 
         { 0x95cbdac0, 0xf74afa69, 0xcebd0e5c, 0x7defbc6f, 0xaf0cbeaf } }
    , { "1f179b3b82250a65e1b0aee949e218e2f45c7a8dbfd6ba08de05c55acfc226b48c68d7f7057e5675cd96fcfc", 
         { 0xf0307bcb, 0x92842e5a, 0xe0cd4f4f, 0x14f3df7f, 0x877fbef2 } }
    , { "ee3d72da3a44d971578972a8e6780ce64941267e0f7d0179b214fa97855e1790e888e09fbe3a70412176cb3b54", 
         { 0x7b13bb0d, 0xbf14964b, 0xd63b133a, 0xc85e2210, 0x0542ef55 } }
    , { "d4d4c7843d312b30f610b3682254c8be96d5f6684503f8fbfbcd15774fc1b084d3741afb8d24aaa8ab9c104f7258", 
         { 0xc314d2b6, 0xcf439be6, 0x78d2a74e, 0x890d96cf, 0xac1c02ed } }
    , { "32c094944f5936a190a0877fb9178a7bf60ceae36fd530671c5b38c5dbd5e6a6c0d615c2ac8ad04b213cc589541cf6", 
         { 0x4d0be361, 0xe410b47a, 0x9d67d8ce, 0x0bb6a8e0, 0x1c53c078 } }
    , { "e5d3180c14bf27a5409fa12b104a8fd7e9639609bfde6ee82bbf9648be2546d29688a65e2e3f3da47a45ac14343c9c02", 
         { 0xe5353431, 0xffae097f, 0x675cbf49, 0x8869f6fb, 0xb6e1c9f2 } }
    , { "e7b6e4b69f724327e41e1188a37f4fe38b1dba19cbf5a7311d6e32f1038e97ab506ee05aebebc1eed09fc0e357109818b9", 
         { 0xb8720a70, 0x68a085c0, 0x18ab1896, 0x1de2765a, 0xa6cd9ac4 } }
    , { "bc880cb83b8ac68ef2fedc2da95e7677ce2aa18b0e2d8b322701f67af7d5e7a0d96e9e33326ccb7747cfff0852b961bfd475", 
         { 0xb0732181, 0x568543ba, 0x85f2b6da, 0x602b4b06, 0x5d9931aa } }
    , { "235ea9c2ba7af25400f2e98a47a291b0bccdaad63faa2475721fda5510cc7dad814bce8dabb611790a6abe56030b798b75c944", 
         { 0x9c22674c, 0xf3222c3b, 0xa9216726, 0x94aafee4, 0xce67b96b } }
    , { "07e3e29fed63104b8410f323b975fd9fba53f636af8c4e68a53fb202ca35dd9ee07cb169ec5186292e44c27e5696a967f5e67709",
         { 0xd128335f, 0x4cecca90, 0x66cdae08, 0x958ce656, 0xff0b4cfc } }
    , { "65d2a1dd60a517eb27bfbf530cf6a5458f9d5f4730058bd9814379547f34241822bf67e6335a6d8b5ed06abf8841884c636a25733f", 
         { 0x0b67c57a, 0xc578de88, 0xa2ae055c, 0xaeaec8bb, 0x9b0085a0 } }
    , { "dcc86b3bd461615bab739d8daafac231c0f462e819ad29f9f14058f3ab5b75941d4241ea2f17ebb8a458831b37a9b16dead4a76a9b0e", 
         { 0xc766f912, 0xa89d4ccd, 0xa88e0cce, 0x6a713ef5, 0xf178b596 } }
    , { "4627d54f0568dc126b62a8c35fb46a9ac5024400f2995e51635636e1afc4373dbb848eb32df23914230560b82477e9c3572647a7f2bb92", 
         { 0x9aa3925a, 0x9dcb177b, 0x15ccff9b, 0x78e70cf3, 0x44858779 } }
    , { "ba531affd4381168ef24d8b275a84d9254c7f5cc55fded53aa8024b2c5c5c8aa7146fe1d1b83d62b70467e9a2e2cb67b3361830adbab28d7", 
         { 0x4811fa30, 0x042fc076, 0xacf37c8e, 0x2274d025, 0x307e5943 } }
    , { "8764dcbcf89dcf4282eb644e3d568bdccb4b13508bfa7bfe0ffc05efd1390be22109969262992d377691eb4f77f3d59ea8466a74abf57b2ef4", 
         { 0x67430184, 0x50c97307, 0x61ee2b13, 0x0df9b91c, 0x1e118150 } }
    , { "497d9df9ddb554f3d17870b1a31986c1be277bc44feff713544217a9f579623d18b5ffae306c25a45521d2759a72c0459b58957255ab592f3be4", 
         { 0x71ad4a19, 0xd37d92a5, 0xe6ef3694, 0xddbeb5aa, 0x61ada645 } }
    , { "72c3c2e065aefa8d9f7a65229e818176eef05da83f835107ba90ec2e95472e73e538f783b416c04654ba8909f26a12db6e5c4e376b7615e4a25819", 
         { 0xa7d9dc68, 0xdacefb7d, 0x61161860, 0x48cb355c, 0xc548e11d } }
    , { "7cc9894454d0055ab5069a33984e2f712bef7e3124960d33559f5f3b81906bb66fe64da13c153ca7f5cabc89667314c32c01036d12ecaf5f9a78de98", 
         { 0x142e429f, 0x0522ba5a, 0xbf5131fa, 0x81df82d3, 0x55b96909 } }
    , { "74e8404d5a453c5f4d306f2cfa338ca65501c840ddab3fb82117933483afd6913c56aaf8a0a0a6b2a342fc3d9dc7599f4a850dfa15d06c61966d74ea59", 
         { 0xef72db70, 0xdcbcab99, 0x1e963797, 0x6c6faf00, 0xd22caae9 } }
    , { "46fe5ed326c8fe376fcc92dc9e2714e2240d3253b105adfbb256ff7a19bc40975c604ad7c0071c4fd78a7cb64786e1bece548fa4833c04065fe593f6fb10", 
         { 0xf220a745, 0x7f4588d6, 0x39dc2140, 0x7c942e98, 0x43f8e26b } }
    , { "836dfa2524d621cf07c3d2908835de859e549d35030433c796b81272fd8bc0348e8ddbc7705a5ad1fdf2155b6bc48884ac0cd376925f069a37849c089c8645", 
         { 0xddd2117b, 0x6e309c23, 0x3ede85f9, 0x62a0c2fc, 0x215e5c69 } }
    , { "7e3a4c325cb9c52b88387f93d01ae86d42098f5efa7f9457388b5e74b6d28b2438d42d8b64703324d4aa25ab6aad153ae30cd2b2af4d5e5c00a8a2d0220c6116", 
         { 0xa3054427, 0xcdb13f16, 0x4a610b34, 0x8702724c, 0x808a0dcc } }
    };

    char const xdigits[17] = "0123456789abcdef";
    char const*const xdigits_end = xdigits+16;

    for (std::size_t i=0; i!=sizeof(cases)/sizeof(cases[0]); ++i) {
        test_case const& tc = cases[i];

        boost::uuids::detail::sha1 sha;
        std::size_t message_length = std::strlen(tc.message);
        BOOST_TEST_EQ(message_length % 2, 0u);

        for (std::size_t b=0; b<message_length; b+=2) {
            char c = tc.message[b];
            char const* f = std::find(xdigits, xdigits_end, c);
            BOOST_TEST_NE(f, xdigits_end);
            
            unsigned char byte = static_cast<unsigned char>(std::distance(&xdigits[0], f));

            c = tc.message[b+1];
            f = std::find(xdigits, xdigits_end, c);
            BOOST_TEST_NE(f, xdigits_end);

            byte <<= 4;
            byte |= static_cast<unsigned char>(std::distance(&xdigits[0], f));

            sha.process_byte(byte);
        }

        unsigned int digest[5];
        sha.get_digest(digest);

        BOOST_TEST_SHA1_DIGEST(digest, tc.digest);
    }
}

// test long strings of 'a's
void test_long()
{

    // test 1 million 'a's
    struct test_case
    {
        unsigned int count;
        unsigned int digest[5];
    };
    test_case cases[] =
    { { 1000000, { 0x34aa973c, 0xd4c4daa4, 0xf61eeb2b, 0xdbad2731, 0x6534016f } }
    , { 1000000000, { 0xd0f3e4f2, 0xf31c665a, 0xbbd8f518, 0xe848d5cb, 0x80ca78f7 } }
    //, { 2000000000, { 0xda19be1b, 0x5ec3bc13, 0xda5533bd, 0x0c225a2f, 0x38da50ed } }
    //, { 2147483647, { 0x1e5b490b, 0x10255e37, 0xfd96d096, 0x4f2fbfb9, 0x1ed47536 } }
    //, { 4294967295 /*2^32 - 1*/, { 0xd84b866e, 0x70f6348f, 0xb0ddc21f, 0x373fe956, 0x1bf1005d } }
    };

    for (size_t iCase=0; iCase<sizeof(cases)/sizeof(cases[0]); iCase++) {
        test_case const& tc = cases[iCase];

        boost::uuids::detail::sha1 sha;
        for (size_t i=0; i<tc.count; i++) {
            sha.process_byte('a');
        }

        unsigned int digest[5];
        sha.get_digest(digest);

        BOOST_TEST_SHA1_DIGEST(digest, tc.digest);
    }
}

int main(int, char*[])
{
    test_quick();
    test_short_messages();
    test_long();
    
    return boost::report_errors();
}
