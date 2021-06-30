#ifndef ARGPARS_HPP
#define ARGPARS_HPP

/* ArgPars, a single-header simple command line argument parser
 *
 * Copyright (c) 2020 Spencer Burton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

//Compilers that don't support string_view will just use std::string
#if __cplusplus >= 201703L
	#include <string_view>
	#define AP_STRING std::string_view
#else
	#define AP_STRING std::string
#endif
 
 
namespace ap {
	
	//--------------- STRUCTS ---------------//
	
	struct Option {
	private:
	
		std::string m_long_name;     //Long names are preceded by two hyphens("--"), ex. --help
		std::string m_short_name;    //Short names are preceded by only one hyphen("-"), -h but can be more than one letter
		std::string m_help_message;  //The help message to be displayed for this option
		std::string m_default_param; //The default parameter for the option when none is provided
		bool m_has_param;            //Determines if the option has a parameter after it, ex. --number 12 <-- parameter
		
	public:
	
		Option() : m_long_name(""), m_short_name(""), m_help_message(""), m_has_param(false) { }
		
		void operator=(Option &other) {
			this->m_long_name    = other.m_long_name;
			this->m_short_name   = other.m_short_name;
			this->m_help_message = other.m_help_message;
			this->m_has_param    = other.m_has_param;
		}
		
		std::string long_name()     const { return m_long_name;     }
		std::string short_name()    const { return m_short_name;    }
		std::string help_message()  const { return m_help_message;  }
		std::string default_param() const { return m_default_param; }
		bool has_param()            const { return m_has_param;     }
		
		friend class Builder;
	};
	
	class Builder {
	private:
	
		Option m_option;
		
	public:
	
		Builder() { }
		Builder& sname(const AP_STRING &short_name) { m_option.m_short_name = short_name; return *this; }
		Builder& lname(const AP_STRING &long_name)  { m_option.m_long_name = long_name;   return *this; }
		Builder& help(const AP_STRING &message)     { m_option.m_help_message = message;  return *this; }
		Builder& param()                            { m_option.m_has_param = true;        return *this; }
		Builder& def_param(const AP_STRING &param)  { m_option.m_default_param = param;   return *this; }
		Option build()                              { return m_option; } //Return the Option with variables set 
	};
	
	struct Options {
	private:

		std::vector<Option> m_options;
		std::vector<std::pair<std::string, std::string>> m_results; //Only holds a result of an option if it was set

	public:
	
		std::vector<std::string> errors; //Holds the error messages, if one occurs
		std::vector<std::string> other_args; //Holds other args that are not options or parameters, i.e. didn't have "-"
		
		size_t num_options() const;
		
		void add_option(Option option);	
		Option get_option(size_t index) const;
		Option get_option(const AP_STRING &name) const;	
		std::string get_param(const AP_STRING &name) const;
		std::string get_param(const Option &option) const;
		std::string get_param_any(const AP_STRING &name) const;
		bool is_set(const AP_STRING &name) const;
		bool is_set(const Option &option) const;
		bool is_set_any(const AP_STRING &name) const;
		
		bool is_error() const;
		
		std::string usage_message(const AP_STRING &name, const AP_STRING &usage, uint32_t option_width = 20) const;
		void parse_args(int argc, char *argv[]);
	};
	
#ifdef ARGPARS_IMPLEMENTATION
	
	//--------------- METHODS ---------------//
	
	size_t Options::num_options() const { return m_options.size(); }
	
	void Options::add_option(Option option) {
		if(option.short_name() != "" || option.long_name() != "")
			m_options.push_back(option);
	}
	
	Option Options::get_option(size_t index) const { return m_options.at(index); }
	Option Options::get_option(const AP_STRING &name) const {
		for(size_t i = 0; i < num_options(); i++) {
			const Option &op = m_options.at(i);
			
			if(op.short_name() == name || op.long_name() == name)
				return m_options.at(i);
		}
		
		return Option();
	}
	
	std::string Options::get_param(const AP_STRING &name) const {
		for(auto const &p : m_results)
			if(name == p.first)
				return p.second;
		
		return "";
	}
	std::string Options::get_param(const Option &option) const {
		for(auto const &p : m_results)
			if(option.short_name() == p.first || option.long_name() == p.first)
				return p.second;
		
		return "";
	}
	std::string Options::get_param_any(const AP_STRING &name) const {
		return get_param(get_option(name));
	}
	
	bool Options::is_set(const AP_STRING &name) const {
		for(auto const &p : m_results)
			if(name == p.first)
				return true;
		
		return false;
	}
	bool Options::is_set(const Option &option) const {
		for(auto const &p : m_results)
			if(option.short_name() == p.first || option.long_name() == p.first)
				return true;
		
		return false;
	}
	bool Options::is_set_any(const AP_STRING &name) const {
		return is_set(get_option(name));
	}
	
	bool Options::is_error() const {
		return !errors.empty();
	}
	
	std::string Options::usage_message(const AP_STRING &name, const AP_STRING &usage, uint32_t option_width) const {
		std::stringstream message;
		AP_STRING spacer = "  ", short_prefix = "-", long_prefix = "--";
		
		message << "Usage: " << name << " " << usage << "\n\n";
		message << "Options:\n";
		
		//Add options
		for(const auto &op : m_options) {
			std::stringstream temp;
			
			
			if(op.short_name() != "")
				temp << spacer << short_prefix << op.short_name();
			if(op.long_name() != "")
				temp << spacer << long_prefix << op.long_name();
			if(op.has_param())
				temp << " <param>";
			
			if(op.help_message() != "") {
				size_t width = temp.str().size();
				
				if(width >= option_width) {
					temp << "\n";
					width = 0;
				}
				
				temp << std::setw(option_width - width) << " " << op.help_message();
			}
			
			message << temp.str();
			message << "\n";
		}
		
		return message.str();
	}
	
	void Options::parse_args(int argc, char *argv[]) {
		bool need_param = false;
		
		this->errors.clear(); //Reset error
		
		for(size_t i = 1; i < argc; i++) {
			AP_STRING argument = argv[i];
			
			//Check for "-" prefix
			if(!need_param && argument.size() >= 2 && argument[0] == '-') {
				AP_STRING argument_name = argument.substr(1);
				
				bool valid = false;
				//Check if the name matches any options
				for(auto const &op : this->m_options) {
					//This allows long arguments to be distinguished from short ones
					bool passed = false;
					std::string long_name = "-";
					long_name += op.long_name();
					
					if(argument_name == op.short_name()) {
						this->m_results.push_back({op.short_name(), op.has_param() ? op.default_param() : ""});
						passed = true;
					}
					if(argument_name == long_name) {
						this->m_results.push_back({op.long_name(), op.has_param() ? op.default_param() : ""});
						passed = true;
					}
					
					if(passed && op.has_param()) {
                        valid = true;
                        
                        if(op.has_param())
                    	    need_param = true;
                    }
				}
				
				if(!valid) {
					std::stringstream ss;
					ss << "Invalid Option: " << argument;
					errors.push_back(ss.str());
				}
			} else {
				//Not an option, check if the last option needs a parameter, if not add to m_other
				if(need_param) {
					this->m_results.back().second = argv[i];
					need_param = false;
				} else {
					this->other_args.push_back(argv[i]);
				}
			}
		}
		
		if(need_param)
			errors.push_back("No Parameter Provided For Last Option");
	}
	
#endif //ARGPARS_IMPLEMENTATION
	
} //namespace ap

#endif //ARGPARS_HPP