#define BOOST_TEST_MODULE "get_mapper_mf_test"
#define BOOST_TEST_NO_LIB

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/type_traits.hpp>

#include <map>

#include "../get_mapper_mf.hpp"
#include "../Particle.hpp"
#include "../Species.hpp"


using namespace ecell4;

BOOST_AUTO_TEST_CASE(get_mapper_mf_test_type)
{
    typedef utils::get_mapper_mf<std::string, std::string>::type string_map_type;
    // typedef std::map<std::string, std::string> expected_map_type;
    typedef boost::unordered_map<std::string, std::string> expected_map_type;

    BOOST_CHECK((boost::is_same<string_map_type, expected_map_type>::value));
}

BOOST_AUTO_TEST_CASE(get_mapper_mf_test_identifier_key)
{
    typedef utils::get_mapper_mf<ParticleID, std::string>::type map_type;
    // typedef boost::unordered_map<ParticleID, std::string> map_type;
    map_type target;
    target[ParticleID()] = "hoge";
    BOOST_CHECK_EQUAL(target[ParticleID()], "hoge");
}

BOOST_AUTO_TEST_CASE(get_mapper_mf_test_species_key)
{
    typedef utils::get_mapper_mf<Species, std::string>::type map_type;
    // typedef boost::unordered_map<Species, std::string> map_type;
    map_type target;
    target[Species("foo")] = "bar";
    BOOST_CHECK_EQUAL(target[Species("foo")], "bar");
}
