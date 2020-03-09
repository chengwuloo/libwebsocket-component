#ifndef MY_HTMLCODE
#define MY_HTMLCODE
	
#include <algorithm>
#include <iostream>
#include <string>

#define array_length(a) (sizeof (a) / sizeof (a)[0])

namespace HTML {

	static struct replace_t {
		std::string src;
		std::string dst;
	}codes[] = {
		{ "&",  "&amp;"  },
		{ "<",  "&lt;"   },
		{ ">",  "&gt;"   },
		{ "\"", "&quot;" },
	};
// 	decodes[] = {
// 		{ "&amp;",   "&" },
// 		{ "&lt;",    "<" },
// 		{ "&gt;",    ">" },
// 		{ "&quot;", "\"" },
// 	};

	static size_t const codes_size = array_length(codes);

	//return value.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;").replace("\"", "&quot;");
	std::string Encode(const std::string& s)
	{
		std::string rs(s);
		for (size_t i = 0; i < codes_size; i++) {

			std::string const& src = codes[i].src;
			std::string const& dst = codes[i].dst;
			std::string::size_type start = rs.find_first_of(src);

			while (start != std::string::npos) {
				rs.replace(start, src.size(), dst);

				start = rs.find_first_of(src, start + dst.size());
			}
		}
		return rs;
	}
	//value.replace("&gt;", ">").replace("&lt;", "<").replace("&quot;", '"').replace("&amp;", "&");
	std::string Decode(const std::string& s)
	{
		std::string rs(s);
		for (size_t i = 0; i < codes_size; i++) {

			std::string const& src = codes[i].dst;
			std::string const& dst = codes[i].src;
			std::string::size_type start = rs.find_first_of(src);

			while (start != std::string::npos) {
				rs.replace(start, src.size(), dst);

				start = rs.find_first_of(src, start + dst.size());
			}
		}
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