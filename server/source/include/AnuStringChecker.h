#pragma once

// 000D, 000A, 0009 : CR, LF, TAB
#define UTF8_IS_CTRL(p)         \
  (((p) == 0x000D)              \
   || ((p) == 0x000A)           \
   || ((p) == 0x0009)           \
  )

// 2E80..2EFF; CJK Radicals Supplement
// 3000..303F; CJK Symbols and Punctuation
// 3040..309F; Hiragana
// 30A0..30FF; Katakana
// 3100..312F; Bopomofo
// 3130..318F; Hangul Compatibility Jamo
// 3190..319F; Kanbun
// 31A0..31BF; Bopomofo Extended
// 31C0..31EF; CJK Strokes
// 31F0..31FF; Katakana Phonetic Extensions
// 3200..32FF; Enclosed CJK Letters and Months
// 3300..33FF; CJK Compatibility
// 3400..4DBF; CJK Unified Ideographs Extension A
// 4DC0..4DFF; Yijing Hexagram Symbols
// 4E00..9FFF; CJK Unified Ideographs
// A700..A71F; Modifier Tone Letters
// AC00..D7AF; Hangul Syllables
// F900..FAFF; CJK Compatibility Ideographs
// FE30..FE4F; CJK Compatibility Forms
// FF00..FFEF; Halfwidth and Fullwidth Forms
// 20000..2A6DF; CJK Unified Ideographs Extension B
// 2F800..2FA1F; CJK Compatibility Ideographs Supplement
#define UTF8_IS_CJK(p)                          \
  (((p) >= 0x1100 && (p) <= 0x11FF)             \
   || ((p) >= 0x2E80 && (p) <= 0x2EFF)          \
   || ((p) >= 0x3000 && (p) <= 0x303F)          \
   || ((p) >= 0x3040 && (p) <= 0x309F)          \
   || ((p) >= 0x30A0 && (p) <= 0x30FF)          \
   || ((p) >= 0x3100 && (p) <= 0x312F)          \
   || ((p) >= 0x3130 && (p) <= 0x318F)          \
   || ((p) >= 0x3190 && (p) <= 0x319F)          \
   || ((p) >= 0x31A0 && (p) <= 0x31BF)          \
   || ((p) >= 0x31C0 && (p) <= 0x31EF)          \
   || ((p) >= 0x31F0 && (p) <= 0x31FF)          \
   || ((p) >= 0x3200 && (p) <= 0x32FF)          \
   || ((p) >= 0x3300 && (p) <= 0x33FF)          \
   || ((p) >= 0x3400 && (p) <= 0x4DBF)          \
   || ((p) >= 0x4DC0 && (p) <= 0x4DFF)          \
   || ((p) >= 0x4E00 && (p) <= 0x9FFF)          \
   || ((p) >= 0xA700 && (p) <= 0xA71F)          \
   || ((p) >= 0xAC00 && (p) <= 0xD7AF)          \
   || ((p) >= 0xF900 && (p) <= 0xFAFF)          \
   || ((p) >= 0xFE30 && (p) <= 0xFE4F)          \
   || ((p) >= 0xFF00 && (p) <= 0xFFEF)          \
   || ((p) >= 0x20000 && (p) <= 0x2A6DF)        \
   || ((p) >= 0x2F800 && (p) <= 0x2FA1F)        \
   || ((p) >= 0x2F800 && (p) <= 0x2FA1F))

// 0041..005A, 0061..007A : English
// 0030..0039 : Numberic
// 1100..11FF, 3131..318F, AC00..D7A3 : Korean
// 3040..309F, 30A0..30FF, 31F0..31FF : Japaneses
// 2E80..2EFF, 3400..3DBF, 4E00..9FFF : Chineses
// 0E00..0E7F : Thai
// 0026, 005F, 003D : & _ =
// 2605, 2606, 2661, 2665, 2667, 2663, 266A, 266C, 2669 : https://www.compart.com/en/unicode/U+2605
#define UTF8_IS_NAMEABLE(p)						\
  (((p) >= 0x0041 && (p) <= 0x005A)             \
   || ((p) >= 0x0061 && (p) <= 0x007A)          \
   || ((p) >= 0x0030 && (p) <= 0x0039)          \
   || ((p) >= 0x1100 && (p) <= 0x11FF)          \
   || ((p) >= 0x3131 && (p) <= 0x318F)          \
   || ((p) >= 0xAC00 && (p) <= 0xD7A3)          \
   || ((p) >= 0x3040 && (p) <= 0x309F)          \
   || ((p) >= 0x30A0 && (p) <= 0x30FF)          \
   || ((p) >= 0x31F0 && (p) <= 0x31FF)          \
   || ((p) >= 0x2E80 && (p) <= 0x2EFF)          \
   || ((p) >= 0x3400 && (p) <= 0x4DBF)          \
   || ((p) >= 0x4E00 && (p) <= 0x9FFF)          \
   || ((p) >= 0x0E00 && (p) <= 0x0E7F)          \
   || ((p) == 0x2605)							\
   || ((p) == 0x2606)							\
   || ((p) == 0x2661)							\
   || ((p) == 0x2665)							\
   || ((p) == 0x2667)							\
   || ((p) == 0x2663)							\
   || ((p) == 0x266A)							\
   || ((p) == 0x266C)							\
   || ((p) == 0x2669)							\
   || ((p) == 0x0026)							\
   || ((p) == 0x005F)							\
   || ((p) == 0x003D)							\
  )

inline bool IsBadCont(unsigned char ch) { return (ch & 0xc0) != 0x80; }

class Utf8Iterator
{
public:
	Utf8Iterator(const char* str, size_t size) {
		_utf8Begin = (unsigned char*)str;
		_utf8Cur = _utf8Begin;
		_utf8End = _utf8Begin + size;
		_utf8Size = size;
		_utf8SeqLen = 0;
	}

	Utf8Iterator& operator++(int) {
		if (_utf8SeqLen == 0) CalcSeqLen();
		_utf8Cur += _utf8SeqLen;

		if (_utf8Cur == _utf8End) _utf8Cur = nullptr;
		_utf8SeqLen = 0;

		return *this;
	}

	bool operator==(const Utf8Iterator &other) const { return _utf8Cur == other._utf8Cur; }
	bool operator!=(const Utf8Iterator &other) const { return _utf8Cur != other._utf8Cur; }

	unsigned int operator*() const {
		if (_utf8Cur == nullptr) return unsigned(-1);
		if (_utf8SeqLen == 0) CalcSeqLen();

		unsigned char ch = *_utf8Cur;
		if (_utf8SeqLen == 1) return ch;
		if (_utf8SeqLen == 2) return ((ch & 0x1f) << 6) | (_utf8Cur[1] & 0x3f);
		if (_utf8SeqLen == 3)
			return ((ch & 0x0f) << 12) | ((_utf8Cur[1] & 0x3f) << 6) | (_utf8Cur[2] & 0x3f);
		return ((ch & 0x07) << 18) | ((_utf8Cur[1] & 0x3f) << 12) |
			((_utf8Cur[2] & 0x3f) << 6) | (_utf8Cur[3] & 0x3f);
	}

	bool IsEnd() {
		return (_utf8Cur == nullptr || *_utf8Cur == 0x0);
	}

	void CalcSeqLen() const {
		unsigned char ch = *_utf8Cur;

		_utf8SeqLen = 1;
		if (ch < 0xc2) return;

		if (ch < 0xe0) {
			if (_utf8Cur + 1 == _utf8End || // Not enough bytes
				IsBadCont(_utf8Cur[1])) // Invalid
				return;
			_utf8SeqLen = 2;
			return;
		}
		if (ch < 0xf0) {
			if (_utf8End - _utf8Cur < 3 || // Not enough bytes
				IsBadCont(_utf8Cur[1]) || IsBadCont(_utf8Cur[2]) || // Invalid
				(_utf8Cur[0] == 0xe0 && _utf8Cur[1] < 0xa0)) // Overlong encoding
				return;
			_utf8SeqLen = 3;
			return;
		}
		if (ch >= 0xf5 || // Code value above Unicode
			_utf8End - _utf8Cur < 4 || // Not enough bytes
			IsBadCont(_utf8Cur[1]) || IsBadCont(_utf8Cur[2]) || IsBadCont(_utf8Cur[3]) || // Invalid
			(_utf8Cur[0] == 0xf0 && _utf8Cur[1] < 0x90) || // Overlong encoding
			(_utf8Cur[0] == 0xf4 && _utf8Cur[1] >= 0x90)) // Code value above Unicode
			return;
		_utf8SeqLen = 4;
		return;
	}

	const unsigned char* _utf8Begin = nullptr;
	const unsigned char* _utf8Cur = nullptr;
	const unsigned char* _utf8End = nullptr;
	mutable size_t		 _utf8SeqLen = 0;
	size_t				 _utf8Size = 0;
};

static size_t GetUIStringSize(const char* utf8Str, size_t strSize)
{
	size_t len = 0;

	for (Utf8Iterator it = Utf8Iterator(utf8Str, strSize); !it.IsEnd(); it++) {
		unsigned int utf8ch = *it;
		if (UTF8_IS_CTRL(utf8ch)) continue;
		if (UTF8_IS_CJK(utf8ch)) {
			len += 2;
		}
		else len += 1;
	}
	return len;
}

static bool IsNameableString(const char* utf8Str, size_t strSize)
{
	for (Utf8Iterator it = Utf8Iterator(utf8Str, strSize); !it.IsEnd(); it++) {
		unsigned int utf8ch = *it;
		if (!UTF8_IS_NAMEABLE(utf8ch)) {
			return false;
		}
	}
	return true;
}

static size_t GetStringLength(const char* utf8Str)
{
	size_t strLength = ::strlen(utf8Str);
	return GetUIStringSize(utf8Str, strLength);
}

enum class NameValidationResult : uint8 
{
	Success,
	Fail_Length,
	Fail_InvalidCharacter
};

static NameValidationResult IsValidNickName(const char* szID)
{
	size_t strLength = ::strlen(szID);
	size_t length = GetUIStringSize(szID, strLength);

	if (length < 4 || length > 12) {
		return NameValidationResult::Fail_Length;
	}

	if (IsNameableString(szID, strLength) == false) {
		return NameValidationResult::Fail_InvalidCharacter;
	}

	return NameValidationResult::Success;
}
