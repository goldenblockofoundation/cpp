////////////////////////////////////////////////////////////////////////////////
//
// Base58Check.h
//
// Copyright (c) 2011-2012 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef BASE58CHECK_H_INCLUDED
#define BASE58CHECK_H_INCLUDED
#include "assert.h"

#include "..\wallet\util\h\BigInt.h"

#include "..\wallet\util\h\encodings.h"

#include "..\wallet\util\h\uchar_vector.h"
#include "..\wallet\util\h\hash.h"
// unsecure versions, suitable for public keys
inline unsigned int countLeading0s(const std::vector<unsigned char>& data)
{
	unsigned int i = 0;
	for (; (i < data.size()) && (data[i] == 0); i++);
	return i;
}

inline unsigned int countLeading0s(const std::string& numeral, char zeroSymbol)
{
	unsigned int i = 0;
	for (; (i < numeral.size()) && (numeral[i] == zeroSymbol); i++);
	return i;
}

inline std::string toBase58Check(const std::vector<unsigned char>& payload, unsigned char version, const char* _base58chars = DEFAULT_BASE58_CHARS)
{
	uchar_vector data;
	data.push_back(version);                                        // prepend version byte
	data += payload;
	uchar_vector checksum = sha256_2(data);
	checksum.assign(checksum.begin(), checksum.begin() + 4);        // compute checksum
	data += checksum;                                               // append checksum
	BigInt bn(data);
	std::string base58check = bn.getInBase(58, _base58chars);             // convert to base58
	std::string leading0s(countLeading0s(data), _base58chars[0]);         // prepend leading 0's (1 in base58)
	return leading0s + base58check;
}

inline std::string toBase58Check(const std::vector<unsigned char>& payload, const std::vector<unsigned char>& version = std::vector<unsigned char>(), const char* _base58chars = DEFAULT_BASE58_CHARS)
{
	uchar_vector data;
	data += version;                                            // prepend version byte
	data += payload;
	uchar_vector checksum = sha256_2(data);
	checksum.assign(checksum.begin(), checksum.begin() + 4);        // compute checksum
	data += checksum;                                               // append checksum
	BigInt bn(data);
	std::string base58check = bn.getInBase(58, _base58chars);             // convert to base58
	std::string leading0s(countLeading0s(data), _base58chars[0]);         // prepend leading 0's (1 in base58)
	return leading0s + base58check;
}

// fromBase58Check() - gets payload and version from a base58check string.
//    returns true if valid.
//    returns false and does not modify parameters if invalid.
inline bool fromBase58Check(const std::string& base58check, std::vector<unsigned char>& payload, unsigned int& version, const char* _base58chars = DEFAULT_BASE58_CHARS)
{
	BigInt bn(base58check, 58, _base58chars);                                // convert from base58
	uchar_vector bytes = bn.getBytes();
	if (bytes.size() < 4) return false;                                     // not enough bytes
	uchar_vector checksum = uchar_vector(bytes.end() - 4, bytes.end());
	bytes.assign(bytes.begin(), bytes.end() - 4);                           // split string into payload part and checksum part
	uchar_vector leading0s(countLeading0s(base58check, _base58chars[0]), 0); // prepend leading 0's
	bytes = leading0s + bytes;
	uchar_vector hashBytes = sha256_2(bytes);
	hashBytes.assign(hashBytes.begin(), hashBytes.begin() + 4);
	if (hashBytes != checksum) return false;                                // verify checksum
	version = bytes[0];
	payload.assign(bytes.begin() + 1, bytes.end());
	return true;
}

inline bool fromBase58Check(const std::string& base58check, std::vector<unsigned char>& payload, const char* _base58chars = DEFAULT_BASE58_CHARS)
{
	BigInt bn(base58check, 58, _base58chars);                                // convert from base58
	uchar_vector bytes = bn.getBytes();
	if (bytes.size() < 4) return false;                                     // not enough bytes
	uchar_vector checksum = uchar_vector(bytes.end() - 4, bytes.end());
	bytes.assign(bytes.begin(), bytes.end() - 4);                           // split string into payload part and checksum part
	uchar_vector leading0s(countLeading0s(base58check, _base58chars[0]), 0); // prepend leading 0's
	bytes = leading0s + bytes;
	uchar_vector hashBytes = sha256_2(bytes);
	hashBytes.assign(hashBytes.begin(), hashBytes.begin() + 4);
	if (hashBytes != checksum) return false;                                // verify checksum
	payload.assign(bytes.begin(), bytes.end());
	return true;
}

inline bool isBase58CheckValid(const std::string& base58check, const char* _base58chars = DEFAULT_BASE58_CHARS)
{
	BigInt bn(base58check, 58, _base58chars);                                // convert from base58
	uchar_vector bytes = bn.getBytes();
	uchar_vector checksum = uchar_vector(bytes.end() - 4, bytes.end());
	bytes.assign(bytes.begin(), bytes.end() - 4);                           // split string into payload part and checksum part
	uchar_vector leading0s(countLeading0s(base58check, _base58chars[0]), 0); // prepend leading 0's
	bytes = leading0s + bytes;
	uchar_vector hashBytes = sha256_2(bytes);
	hashBytes.assign(hashBytes.begin(), hashBytes.begin() + 4);
	return (hashBytes == checksum);                                         // verify checksum
}




inline bool DecodeBase58(const char *psz, std::vector<unsigned char>& vch)
{
	// Skip leading spaces.
	while (*psz && isspace(*psz))
		psz++;
	// Skip and count leading '1's.
	int zeroes = 0;
	while (*psz == '1') {
		zeroes++;
		psz++;
	}
	// Allocate enough space in big-endian base256 representation.
	std::vector<unsigned char> b256(strlen(psz) * 733 / 1000 + 1); // log(58) / log(256), rounded up.
																   // Process the characters.
	while (*psz && !isspace(*psz)) {
		// Decode base58 character
		const char *ch = strchr(BITCOIN_BASE58_CHARS, *psz);
		if (ch == NULL)
			return false;
		// Apply "b256 = b256 * 58 + ch".
		int carry = ch - BITCOIN_BASE58_CHARS;
		for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); it != b256.rend(); it++) {
			carry += 58 * (*it);
			*it = carry % 256;
			carry /= 256;
		}
		assert(carry == 0);
		psz++;
	}
	// Skip trailing spaces.
	while (isspace(*psz))
		psz++;
	if (*psz != 0)
		return false;
	// Skip leading zeroes in b256.
	std::vector<unsigned char>::iterator it = b256.begin();
	while (it != b256.end() && *it == 0)
		it++;
	// Copy result into output vector.
	vch.reserve(zeroes + (b256.end() - it));
	vch.assign(zeroes, 0x00);
	while (it != b256.end())
		vch.push_back(*(it++));
	return true;
}





// and secure versions, suitable for private keys - Not done yet
// Should use templates.
/*
unsigned int countLeading0s(const uchar_vector_secure& data)
{
unsigned int i = 0;
for (; (i < data.size()) && (data[i] == 0); i++);
return i;
}
unsigned int countLeading0s(const std::string_secure& numeral, char zeroSymbol)
{
unsigned int i = 0;
for (; (i < numeral.size()) && (numeral[i] == zeroSymbol); i++);
return i;
}
string_secure toBase58Check(const uchar_vector_secure& payload, unsigned int version)
{
uchar_vector_secure data;
data.push_back(version);                                        // prepend version byte
data += payload;
uchar_vector_secure checksum = sha256_2(data);
checksum.assign(checksum.begin(), checksum.begin() + 4);        // compute checksum
data += checksum;                                               // append checksum
BigInt bn(data);
string_secure base58check = bn.getInBase(58, base58chars);             // convert to base58
string_secure leading0s(countLeading0s(data), base58chars[0]);         // prepend leading 0's (1 in base58)
return leading0s + base58check;
}
// fromBase58Check() - gets payload and version from a base58check string.
//    returns true if valid.
//    returns false and does not modify parameters if invalid.
bool fromBase58Check(const string_secure& base58check, uchar_vector_secure& payload, unsigned int& version)
{
BigInt bn(base58check, 58, base58chars);                                // convert from base58
uchar_vector_secure bytes = bn.getBytes();
uchar_vector_secure checksum = uchar_vector(bytes.end() - 4, bytes.end());
bytes.assign(bytes.begin(), bytes.end() - 4);                           // split string into payload part and checksum part
uchar_vector_secure leading0s(countLeading0s(base58check, base58chars[0]), 0); // prepend leading 0's
bytes = leading0s + bytes;
uchar_vector_secure hashBytes = sha256_2(bytes);
hashBytes.assign(hashBytes.begin(), hashBytes.begin() + 4);
if (hashBytes != checksum) return false;                                // verify checksum
version = bytes[0];
payload.assign(bytes.begin() + 1, bytes.end());
return true;
}*/

#endif // BASE58CHECK_H_INCLUDED