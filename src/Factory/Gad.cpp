#include "Factory/Gad.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Gad_name   = "Gad";
const std::string aff3ct::factory::Gad_prefix = "gad";


Gad
::Gad(const std::string &prefix)
: Factory(Gad_name, Gad_name, prefix)
{
}

Gad* Gad
::clone() const
{
	return new Gad(*this);
}

void Gad
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	const std::string class_name = "factory::Gad::";

    args.add({p+"-ebn0"  ,"s" }, cli::Real()                  , "EbN0 simulated"                             );
    args.add({p+"-fb-ebn0"    }, cli::Real()                  , "EbN0 used to generate fb"                   );
    args.add({p+"-range" ,"R" }, cli::Integer()               , "Shuffle range"                              );
    args.add({p+"-size"  ,"S" }, cli::Integer()               , "Dataset size"                               );
    args.add({p+"-thread","t" }, cli::Integer()               , "Dataset size"                               );
    args.add({p+"-seed"       }, cli::Integer(cli::Positive()), "Seed for frozen bits shuffling"             );
	args.add({p+"-term"       }, cli::None()                  , "Enable terminal"                            );
	args.add({p+"-polar-rm"   }, cli::None()                  , "Use Polar-RM construction"                  );
	args.add({p+"-fb-file","F"}, cli::Text()                  , "Fb are not generated but read in given file");
}

void Gad
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();

	if (vals.exist({p+"-ebn0" ,"s"  })) this->ebn0            = vals.to_float({p+"-ebn0" ,"s" });
	if (vals.exist({p+"-range","R"  })) this->shuffle_range   = vals.to_int  ({p+"-range","R" });
	if (vals.exist({p+"-size" ,"S"  })) this->dataset_size    = vals.to_int  ({p+"-size" ,"S" });
	if (vals.exist({p+"-thread","t" })) this->n_threads       = vals.to_int  ({p+"-thread","t"});
	if (vals.exist({p+"-seed"       })) this->seed            = vals.to_int  ({p+"-seed"      });
	if (vals.exist({p+"-term"       })) this->enable_terminal = true;
	if (vals.exist({p+"-polar-rm"   })) this->polar_rm        = true;
	if (vals.exist({p+"-fb-file","F"})) this->fb_file         = vals.at({p+"-fb-file","F"});

	this->fb_ebn0 = vals.exist({p+"-fb-ebn0"    }) ? vals.to_float({p+"-fb-ebn0"    }) : this->ebn0;
}

void Gad
::get_headers(std::map<std::string,tools::header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

    headers[p].push_back(std::make_pair("Eb/N0"        , std::to_string(this->ebn0)             ));
    headers[p].push_back(std::make_pair("Shuffle range", std::to_string(this->shuffle_range)    ));
    headers[p].push_back(std::make_pair("Dataset size" , std::to_string(this->dataset_size)     ));
    headers[p].push_back(std::make_pair("Thread number", std::to_string(this->n_threads)        ));
    headers[p].push_back(std::make_pair("Shuffle seed" , std::to_string(this->seed)             ));
    headers[p].push_back(std::make_pair("Term"         , this->enable_terminal ? "on" : "off"   ));
    headers[p].push_back(std::make_pair("Polar RM"     , this->polar_rm ? "enabled" : "disabled"));
	if (this->fb_ebn0 != this->ebn0)
		headers[p].push_back(std::make_pair("FB ebn0"  , std::to_string(this->fb_ebn0)));
	if (!this->fb_file.empty())
    	headers[p].push_back(std::make_pair("FB file"  , this->fb_file                ));
}
