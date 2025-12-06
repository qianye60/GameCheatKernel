/*
    By 开源 公益分享
   搜集各个公益文件分享
    更多公益开源 文件 @BYYXnb
    TG @BYYXnb QQ群 427975011
    
源码捐赠链接
https://v.youxi186.com/shop/YIZ6ZA6X

源码捐赠地址2：
https://shop.pp13.cn/shop/byyxnb

公益网盘资源分享
https://www.123865.com/s/NxyZVv-K44U3?提取码：QWT6

QQ群过验证[有的文件需要]
https://www.123865.com/s/f4A7Vv-Pqc0A?提取码：9IVI

由于易支付漏洞 如果进不去
如您需拿卡 点击 联系我
赞助链接已重新更新 

〖如果你点开提示 站点停止运行或者进不去，那么解决办法有两个

1.复制链接去别的浏览器打开
2.清除你浏览器的数据即可
*/
/* --------------------------------- ABOUT -------------------------------------

Original Author: Adam Yaxley
Website: https://github.com/adamyaxley
License: See end of file

Obfuscate
Guaranteed compile-time string literal obfuscation library for C++14

Usage:
Pass string literals into the AY_OBFUSCATE macro to obfuscate them at compile
time. AY_OBFUSCATE returns a reference to an ay::OBFUSCATE_data object with the
following traits:
	- Guaranteed obfuscation of string
	The passed string is encrypted with a simple XOR cipher at compile-time to
	prevent it being viewable in the binary image
	- Global lifetime
	The actual instantiation of the ay::OBFUSCATE_data takes place inside a
	lambda as a function level static
	- Implicitly convertable to a char*
	This means that you can pass it directly into functions that would normally
	take a char* or a const char*

Example:
const char* OBFUSCATE_string = AY_OBFUSCATE("Hello World");
std::cout << OBFUSCATE_string << std::endl;

----------------------------------------------------------------------------- */

#include <cstddef>
#include <string>
namespace ay
{
	// Obfuscates a string at compile time
	template <std::size_t N, char KEY>
	class obfuscator
	{
	public:
		// Obfuscates the string 'data' on construction
		constexpr obfuscator(const char* data)
		{
			static_assert(KEY != '\0', "KEY must not be the null character.");

			// On construction each of the characters in the string is
			// OBFUSCATE with an XOR cipher based on KEY
			for (std::size_t i = 0; i < N; i++)
			{
				m_data[i] = data[i] ^ KEY;
			}
		}

		constexpr const char* getData() const
		{
			return &m_data[0];
		}

		constexpr std::size_t getSize() const
		{
			return N;
		}

		constexpr char getKey() const
		{
			return KEY;
		}

	private:

		char m_data[N]{};
	};

	// Handles decryption and re-encryption of an encrypted string at runtime
	template <std::size_t N, char KEY>
	class OBFUSCATE_data
	{
	public:
		OBFUSCATE_data(const obfuscator<N, KEY>& obfuscator)
		{
			for (std::size_t i = 0; i < N; i++)
			{
				m_data[i] = obfuscator.getData()[i];
			}
		}

		~OBFUSCATE_data()
		{
			// Zero m_data to remove it from memory
			for (std::size_t i = 0; i < N; i++)
			{
				m_data[i] = 0;
			}
		}

		// Returns a pointer to the plain text string, decrypting it if
		// necessary
		operator char*()
		{
			decrypt();
			return m_data;
		}
		operator std::string()
		{
			decrypt();
			return m_data;
		}
		// Manually decrypt the string
		void decrypt()
		{
			if (is_encrypted())
			{
				for (std::size_t i = 0; i < N; i++)
				{
					m_data[i] ^= KEY;
				}
			}
		}

		// Manually re-encrypt the string
		void encrypt()
		{
			if (!is_encrypted())
			{
				for (std::size_t i = 0; i < N; i++)
				{
					m_data[i] ^= KEY;
				}
			}
		}

		// Returns true if this string is currently encrypted, false otherwise.
		bool is_encrypted() const
		{
			return m_data[N - 1] != '\0';
		}

	private:

		// Local storage for the string. Call is_encrypted() to check whether or
		// not the string is currently OBFUSCATE.
		char m_data[N];
	};

	// This function exists purely to extract the number of elements 'N' in the
	// array 'data'
	template <std::size_t N, char KEY = '.'>
	constexpr auto make_obfuscator(const char(&data)[N])
	{
		return obfuscator<N, KEY>(data);
	}
}

// Obfuscates the string 'data' at compile-time and returns a reference to a
// ay::OBFUSCATE_data object with global lifetime that has functions for
// decrypting the string and is also implicitly convertable to a char*
#define OBFUSCATE(data) OBFUSCATE_KEY(data, '.')

// Obfuscates the string 'data' with 'key' at compile-time and returns a
// reference to a ay::OBFUSCATE_data object with global lifetime that has
// functions for decrypting the string and is also implicitly convertable to a
// char*
#define OBFUSCATE_KEY(data, key) \
	[]() -> ay::OBFUSCATE_data<sizeof(data)/sizeof(data[0]), key>& { \
		constexpr auto n = sizeof(data)/sizeof(data[0]); \
		static_assert(data[n - 1] == '\0', "String must be null terminated"); \
		constexpr auto obfuscator = ay::make_obfuscator<n, key>(data); \
		static auto OBFUSCATE_data = ay::OBFUSCATE_data<n, key>(obfuscator); \
		return OBFUSCATE_data; \
	}()
	
/* -------------------------------- LICENSE ------------------------------------

Public Domain (http://www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

----------------------------------------------------------------------------- */
