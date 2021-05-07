/*!
 * \file
 * \brief Class: factory::gad
 */
#ifndef FACTORY_GAD_HPP
#define FACTORY_GAD_HPP

#include <aff3ct.hpp>


namespace aff3ct
{
namespace factory
{

extern const std::string Gad_name;
extern const std::string Gad_prefix;

class Gad : public Factory
{
public:
	bool        enable_terminal = false;
    int         seed            = 0;
    int         shuffle_range   = 2;
    int         dataset_size    = 10;
    int         n_threads       = std::thread::hardware_concurrency();
    float       ebn0            = 3.f;
	float       fb_ebn0         = 3.f;
	bool        polar_rm        = false;
	std::string fb_file         = "";

	explicit Gad(const std::string &p = Gad_prefix);
	virtual ~Gad() = default;
	Gad* clone() const;

	// parameters construction
	void get_description(cli::Argument_map_info &args) const;
	void store          (const cli::Argument_map_value &vals);
	void get_headers    (std::map<std::string,tools::header_list>& headers, const bool full = true) const;
};
}
}

#endif /* FACTORY_GAD_HPP */
