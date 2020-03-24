#ifndef MY_HTMLCODE
#define MY_HTMLCODE
	
#include <algorithm>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#define array_length(a) (sizeof (a) / sizeof (a)[0])

//https://www.runoob.com/charsets/ref-html-entities-4.html
//https://tool.chinaz.com/tools/htmlencode.aspx

namespace HTML {
	//字符标记 &...;
	//DEC标记  &#...;
	//HEX标记  &#x...;
	static struct replace_t {
		std::string src;
		std::string dst;
	}codes[] = {
		{ "&",  "&amp;"  },
		{ "<",  "&lt;"   },
		{ ">",  "&gt;"   },
		{ "\"", "&quot;" },
		{ "'",  "&apos;" },
		{ "©",  "&copy;" },
		//{ "#",  "&num;"  },
		{ "§",  "&sect;" },
		{ "¥",  "&yen;"  },
		//{ "$",  "&dollar;" },
		{ "£",  "&pound;"  },
		{ "¢",  "&cent;"   },
		//{ "%",  "&percnt;" },
		//{ "*",  "$ast;"    },
		//{ "@",  "&commat;" },
		//{ "^",  "&Hat;"    },
		{ "±",  "&plusmn;" },
		{ " ",  "&nbsp;"   },
		{ "·",  "&middot;" },
		{ "¸",  "&cedil;"  },
		{ "¦",  "&brvbar;" },
		{ "¤",  "&curren;" },
		{ "¡",  "&iexcl;"  },
		{ "¨",  "&uml;"    },
		{ "«",  "&laquo;"  },
		{ "»",  "&raquo;"  },
		{ "®",  "&reg;"    },
		{ "€",  "&euro;"   },
		{ "™",  "&trade;"  },
		{ "←",  "&larr;"   },
		{ "↑",  "&uarr;"   },
		{ "→",  "&rarr;"   },
		{ "↓",  "&darr;"   },
		{ "↔",  "&harr;"   },
		{ "‾",  "&oline;"  },
		{ "⁄",  "&frasl;"  },
		{ "•",  "&bull;"   },
		{ "…",  "&hellip;" },
		{ "′",  "&prime;"  },
		{ "″",  "&Prime;"  },
		{ "♠",  "&spades;" },
		{ "♥",  "&hearts;" },
		{ "♣",  "&clubs;"  },
		{ "♦",  "&diams;"  },
		{ "Α",  "&Alpha;"  },
		{ "Β",  "&Beta;"   },
		{ "Γ",  "&Gamma;"  },
		{ "Δ",  "&Delta;"  },
		{ "Ε",  "&Epsilon;"},
		{ "Ζ",  "&Zeta;"   },
		{ "¯",  "&macr;"   },
		{ "¬",  "&not;"    },
		{ "°",  "&deg;"    },
		{ "¶",  "&para;"   },
		{ "µ",  "&micro;"  },
		{ "´",  "&acute;"  },
		{ "−",  "&minus;"  },
		{ "×",  "&times;"  },
		{ "÷",  "&divide;" },
		{ "∧",  "&and;"    },
		{ "∨",  "&or;"     },
		{ "≈",  "&asymp;"  },
		{ "≠",  "&ne;"     },
		{ "≡",  "&equiv;"  },
	};
	
	static size_t const codes_size = array_length(codes);

	//return value.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;").replace("\"", "&quot;");
	std::string Encode(const std::string& s)
	{
		std::string rs(s);
#if 0
		for (size_t i = 0; i < codes_size; ++i) {

			std::string const& src = codes[i].src;
			std::string const& dst = codes[i].dst;
			std::string::size_type start = rs.find_first_of(src);

			while (start != std::string::npos) {
				rs.replace(start, src.size(), dst);

				start = rs.find_first_of(src, start + dst.size());
			}
		}
#else
		for (size_t i = 0; i < codes_size; ++i) {
			boost::replace_all<std::string>(rs, codes[i].src, codes[i].dst);
		}
#endif
		return rs;
	}
	//value.replace("&gt;", ">").replace("&lt;", "<").replace("&quot;", '"').replace("&amp;", "&");
	std::string Decode(const std::string& s)
	{
		std::string rs(s);
#if 0
		for (size_t i = 0; i < codes_size; ++i) {

			std::string const& src = codes[i].dst;
			std::string const& dst = codes[i].src;
			std::string::size_type start = rs.find_first_of(src);

			while (start != std::string::npos) {
				rs.replace(start, src.size(), dst);

				start = rs.find_first_of(src, start + dst.size());
			}
		}
#else
		for (size_t i = 0; i < codes_size; ++i) {
			boost::replace_all<std::string>(rs, codes[i].dst, codes[i].src);
		}
#endif
		return rs;
	}
}

#if 0
int main()
{
	std::cout << HTML::Encode("template <class T> void foo( const string& bar );") << '\n';
	return 0;
}
#endif

#endif