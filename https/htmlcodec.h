#ifndef HTMLCODE_INCLUDE_H
#define HTMLCODE_INCLUDE_H
	
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
		{ " ",  "&nbsp;" },
		{ "©",  "&copy;" },
		//{ "#",  "&num;"  },
//		{ "§",  "&sect;" },
//		{ "¥",  "&yen;"  },
		//{ "$",  "&dollar;" },
//		{ "£",  "&pound;"  },
//		{ "¢",  "&cent;"   },
		//{ "%",  "&percnt;" },
		//{ "*",  "$ast;"    },
		//{ "@",  "&commat;" },
		//{ "^",  "&Hat;"    },
//		{ "±",  "&plusmn;" },
//		{ "·",  "&middot;" },
//		{ "¸",  "&cedil;"  },
//		{ "¦",  "&brvbar;" },
//		{ "¤",  "&curren;" },
//		{ "¡",  "&iexcl;"  },
//		{ "¨",  "&uml;"    },
//		{ "«",  "&laquo;"  },
//		{ "»",  "&raquo;"  },
//		{ "®",  "&reg;"    },
//		{ "€",  "&euro;"   },
//		{ "™",  "&trade;"  },
//		{ "←",  "&larr;"   },
//		{ "↑",  "&uarr;"   },
//		{ "→",  "&rarr;"   },
//		{ "↓",  "&darr;"   },
//		{ "↔",  "&harr;"   },
//		{ "‾",  "&oline;"  },
//		{ "⁄",  "&frasl;"  },
//		{ "•",  "&bull;"   },
//		{ "…",  "&hellip;" },
		{ "′",  "&prime;"  },
		{ "″",  "&Prime;"  },
//		{ "♠",  "&spades;" },
//		{ "♥",  "&hearts;" },
//		{ "♣",  "&clubs;"  },
//		{ "♦",  "&diams;"  },
//		{ "¯",  "&macr;"   },
//		{ "¬",  "&not;"    },
//		{ "­",  "&shy;"    },
//		{ "°",  "&deg;"    },
//		{ "¶",  "&para;"   },
//		{ "µ",  "&micro;"  },
//		{ "´",  "&acute;"  },
		{ "−",  "&minus;"  },
//		{ "×",  "&times;"  },
		{ "÷",  "&divide;" },
		{ "∧",  "&and;"    },
		{ "∨",  "&or;"     },
//		{ "≈",  "&asymp;"  },
//		{ "≠",  "&ne;"     },
//		{ "≡",  "&equiv;"  },
//		{ "ª",  "&ordf;"   },
//		{ "²",  "&sup2;"   },
//		{ "³",  "&sup3;"   },
//		{ "¹",  "&sup1;"   },
//		{ "º",  "&ordm;"   },
//		{ "¼",  "&frac14;" },
//		{ "½",  "&frac12;" },
//		{ "¾",  "&frac34;" },
//		{ "¿",  "&iquest;" },
//		{ "〈",  "&lang;"   },
//		{ "〉",  "&rang;"   },
//		{ "◊",  "&loz;"    },
//		{ "α",  "&alpha;"  },
//		{ "β",  "&beta;"   },
//		{ "γ",  "&gamma;"  },
//		{ "δ",  "&delta;"  },
//		{ "ε",  "&epsilon;"},
//		{ "ζ",  "&zeta;"   },
//		{ "η",  "&eta;"    },
//		{ "θ",  "&theta;"  },
//		{ "ι",  "&iota;"   },
//		{ "κ",  "&kappa;"  },
//		{ "λ",  "&lambda;" },
//		{ "μ",  "&mu;"     },
//		{ "ν",  "&nu;"     },
//		{ "ξ",  "&xi;"     },
//		{ "ο",  "&omicron;"},
//		{ "π",  "&pi;"     },
//		{ "ρ",  "&rho;"    },
//		{ "ς",  "&sigmaf;" },
//		{ "σ",  "&sigma;"  },
//		{ "τ",  "&tau;"    },
//		{ "υ",  "&upsilon;"},
//		{ "φ",  "&phi;"    },
//		{ "χ",  "&chi;"    },
//		{ "ψ",  "&psi;"    },
//		{ "ω",  "&omega;"  },
//		{ "ϑ",  "&thetasym;"},
//		{ "ϒ",  "&upsih;"   },
//		{ "ϖ",  "&piv;"     },
//		{ "⌈",  "&lceil;"   },
//		{ "⌉",  "&rceil;"   },
//		{ "⌊",  "&lfloor;"  },
//		{ "⌋",  "&rfloor;"  },
//		{ "‰",  "&permil;"  },
//		{ "‹",  "&lsaquo;"  },
//		{ "›",  "&rsaquo;"  },
//		{ "ℑ",  "&image;"   },
//		{ "℘",  "&weierp;"  },
//		{ "ℜ",  "&real;"    },
//		{ "ℵ",  "&alefsym;" },
//		{ "↵",  "&crarr;"   },
//		{ "⇐",  "&lArr;"    },
//		{ "⇑",  "&uArr;"    },
//		{ "⇒",  "&rArr;"    },
//		{ "⇓",  "&dArr;"    },
//		{ "⇔",  "&hArr;"    },
//		{ "∀",  "&forall;"  },
//		{ "∂",  "&part;"    },
//		{ "∃",  "&exist;"   },
//		{ "∅",  "&empty;"   },
//		{ "∇",  "&nabla;"   },
//		{ "∈",  "&isin;"   },
//		{ "∉",  "&notin;"   },
//		{ "∋",  "&ni;"      },
//		{ "∏",  "&prod;"    },
//		{ "∑",  "&sum;"     },
//		{ "∗",  "&lowast;"  },
//		{ "√",  "&radic;"   },
//		{ "∝",  "&prop;"   },
//		{ "∞",  "&infin;"   },
//		{ "∠",  "&ang;"    },
//		{ "∩",  "&cap;"     },
//		{ "∪",  "&cup;"     },
//		{ "∫",  "&int;"     },
//		{ "∴",  "&there4;"  },
//		{ "∼",  "&sim;"     },
//		{ "≅",  "&cong;"    },
//		{ "≤",  "&le;"      },
//		{ "≥",  "&ge;"      },
//		{ "⊂",  "&sub;"     },
//		{ "⊃",  "&sup;"     },
//		{ "⊄",  "&nsub;"    },
//		{ "⊆",  "&sube;"    },
//		{ "⊇",  "&supe;"    },
//		{ "⊕",  "&oplus;"  },
//		{ "⊗",  "&otimes;"  },
		{ "⊥",  "&perp;"    },
//		{ "⋅",  "&sdot;"    },
		{ "–",  "&ndash;"   },
		{ "—",  "&mdash;"   },
		{ "‘",  "&lsquo;"   },
		{ "’",  "&rsquo;"   },
		{ "‚",  "&sbquo;"   },
		{ "“",  "&ldquo;"   },
		{ "”",  "&rdquo;"   },
		{ "„",  "&bdquo;"   },
//		{ "†",  "&dagger;"  },
//		{ "‡",  "&Dagger;"  },
//		{ "À",  "&Agrave;"  },
//		{ "Á",  "&Aacute;"  },
//		{ "Â",  "&Acirc;"   },
//		{ "Ã",  "&Atilde;"  },
//		{ "Ä",  "&Auml;"    },
//		{ "Å",  "&Aring;"   },
//		{ "Æ",  "&AElig;"   },
//		{ "Ç",  "&Ccedil;"  },
//		{ "È",  "&Egrave;"  },
//		{ "É",  "&Eacute;"  },
//		{ "Ê",  "&Ecirc;"   },
//		{ "Ë",  "&Euml;"    },
//		{ "Ì",  "&Igrave;"  },
//		{ "Í",  "&Iacute;"  },
//		{ "Î",  "&Icirc;"   },
//		{ "Ï",  "&Iuml;"    },
//		{ "Ð",  "&ETH;"     },
//		{ "Ñ",  "&Ntilde;"  },
//		{ "Ò",  "&Ograve;"  },
//		{ "Ó",  "&Oacute;"  },
//		{ "Ô",  "&Ocirc;"   },
//		{ "Õ",  "&Otilde;"  },
//		{ "Ö",  "&Ouml;"    },
//		{ "Ø",  "&Oslash;"  },
//		{ "Ù",  "&Ugrave;"  },
//		{ "Ú",  "&Uacute;"  },
//		{ "Û",  "&Ucirc;"   },
//		{ "Ü",  "&Uuml;"    },
//		{ "Ý",  "&Yacute;"  },
//		{ "Þ",  "&THORN;"   },
//		{ "ß",  "&szlig;"   },
//		{ "à",  "&agrave;"  },
//		{ "á",  "&aacute;"  },
//		{ "â",  "&acirc;"   },
//		{ "ã",  "&atilde;"  },
//		{ "ä",  "&auml;"    },
//		{ "å",  "&aring;"   },
//		{ "æ",  "&aelig;"   },
//		{ "ç",  "&ccedil;"  },
//		{ "è",  "&egrave;"  },
//		{ "é",  "&eacute;"  },
//		{ "ê",  "&ecirc;"   },
//		{ "ë",  "&euml;"    },
//		{ "ì",  "&igrave;"  },
//		{ "í",  "&iacute;"  },
//		{ "î",  "&icirc;"   },
//		{ "ï",  "&iuml;"    },
//		{ "ð",  "&eth;"     },
//		{ "ñ",  "&ntilde;"  },
//		{ "ò",  "&ograve;"  },
//		{ "ó",  "&oacute;"  },
//		{ "ô",  "&ocirc;"   },
//		{ "õ",  "&otilde;"  },
//		{ "ö",  "&ouml;"    },
//		{ "ø",  "&oslash;"  },
//		{ "ù",  "&ugrave;"  },
//		{ "ú",  "&uacute;"  },
//		{ "û",  "&ucirc;"   },
//		{ "ü",  "&uuml;"    },
//		{ "ý",  "&yacute;"  },
//		{ "þ",  "&thorn;"   },
//		{ "ÿ",  "&yuml;"    },
//		{ "Œ",  "&OElig;"   },
//		{ "œ",  "&oelig;"   },
//		{ "Š",  "&Scaron;"  },
//		{ "š",  "&scaron;"  },
//		{ "Ÿ",  "&Yuml;"    },
//		{ "ƒ",  "&fnof;"    },
//		{ "Α",  "&Alpha;"   },
//		{ "Β",  "&Beta;"    },
//		{ "Γ",  "&Gamma;"   },
//		{ "Δ",  "&Delta;"   },
//		{ "Ε",  "&Epsilon;" },
//		{ "Ζ",  "&Zeta;"    },
//		{ "Η",  "&Eta;"     },
//		{ "Θ",  "&Theta;"   },
//		{ "Ι",  "&Iota;"    },
//		{ "Κ",  "&Kappa;"   },
//		{ "Λ",  "&Lambda;"  },
//		{ "Μ",  "&Mu;"      },
//		{ "Ν",  "&Nu;"      },
//		{ "Ξ",  "&Xi;"      },
//		{ "Ο",  "&Omicron;" },
//		{ "Π",  "&Pi;"      },
//		{ "Ρ",  "&Rho;"     },
//		{ "Σ",  "&Sigma;"   },
//		{ "Τ",  "&Tau;"     },
//		{ "Υ",  "&Upsilon;" },
//		{ "Φ",  "&Phi;"     },
//		{ "Χ",  "&Chi;"     },
//		{ "Ψ",  "&Psi;"     },
//		{ "Ω",  "&Omega;"   },
	};
	
	static size_t const codes_size = array_length(codes);

	//return value.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;").replace("\"", "&quot;");
	static std::string Encode(const std::string& s)
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
	static std::string Decode(const std::string& s)
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