/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
# All rights reserved.

# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/


#ifndef _FDVCOLLECTIONS_H_
#define _FDVCOLLECTIONS_H_


#include "fdv.h"


namespace fdv
{



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// ChunkedBuffer

template <typename T>
struct ChunkedBuffer
{

	struct Chunk
	{
		Chunk*   next;
		uint32_t items;
		T*       data;
		bool     freeOnDestroy;	// free "data" on destroy
		
		Chunk(uint32_t capacity)
			: next(NULL), items(0), data(new T[capacity]), freeOnDestroy(true)
		{
		}
		Chunk(T* data_, uint32_t items_, bool freeOnDestroy_)
			: next(NULL), items(items_), data(data_), freeOnDestroy(freeOnDestroy_)
		{
		}
		~Chunk()
		{
			if (freeOnDestroy)
				delete[] data;
		}
	};
	
	
	struct Iterator
	{
		Iterator(Chunk* chunk = NULL)
			: m_chunk(chunk), m_pos(0), m_absPos(0)
		{
		}
		T& MTD_FLASHMEM operator*()
		{			
			return m_chunk->data[m_pos];
		}
		MTD_FLASHMEM operator bool()
		{
			return m_chunk != NULL;
		}
		Iterator MTD_FLASHMEM operator++(int)
		{
			Iterator c = *this;
			next();
			return c;
		}
		Iterator& MTD_FLASHMEM operator++()
		{
			next();
			return *this;
		}
		Iterator& MTD_FLASHMEM operator+=(int32_t rhs)
		{
			while (rhs-- > 0)
				next();
			return *this;
		}
		bool MTD_FLASHMEM operator==(Iterator const& rhs)
		{
			return m_chunk == rhs.m_chunk && m_pos == rhs.m_pos;
		}
		bool MTD_FLASHMEM operator!=(Iterator const& rhs)
		{
			return m_chunk != rhs.m_chunk || m_pos != rhs.m_pos;
		}
		T* MTD_FLASHMEM dup()
		{
			return t_strdup(*this);
		}
		uint32_t getPosition()
		{
			return m_absPos;
		}
		bool isLast()
		{
			return m_chunk->next == NULL && m_pos + 1 >= m_chunk->items;
		}
	private:
		void MTD_FLASHMEM next()
		{
			++m_absPos;
			++m_pos;
			if (m_pos == m_chunk->items)
			{
				m_pos = 0;
				m_chunk = m_chunk->next;
			}
		}
	private:
		Chunk*   m_chunk;
		uint32_t m_pos;  	// position inside this chunk
		uint32_t m_absPos;	// absolute position (starting from beginning of ChunkedBuffer)
	};

	
	ChunkedBuffer()
		: m_chunks(NULL), m_current(NULL)
	{
	}
	
	
	~ChunkedBuffer()
	{
		clear();
	}
	
	
	void MTD_FLASHMEM clear()
	{
		Chunk* chunk = m_chunks;
		while (chunk)
		{
			Chunk* next = chunk->next;
			delete chunk;
			chunk = next;
		}
		m_chunks = m_current = NULL;
	}
	
	
	Chunk* MTD_FLASHMEM addChunk(uint32_t capacity)
	{
		Chunk* newChunk = new Chunk(capacity);
		if (m_chunks == NULL)
			m_current = m_chunks = newChunk;
		else
			m_current = m_current->next = newChunk;
		return newChunk;
	}
	
	
	Chunk* MTD_FLASHMEM addChunk(T* data, uint32_t items, bool freeOnDestroy)
	{
		Chunk* newChunk = new Chunk(data, items, freeOnDestroy);
		if (m_chunks == NULL)
			m_current = m_chunks = newChunk;
		else
			m_current = m_current->next = newChunk;
		return m_current;
	}
	
	
	void MTD_FLASHMEM append(T const& value)
	{
		Chunk* chunk = addChunk(sizeof(T));
		chunk->data[0] = value;
		chunk->items = 1;
	}
	
	
	Chunk* MTD_FLASHMEM getFirstChunk()
	{
		return m_chunks;
	}
	

	Iterator MTD_FLASHMEM getIterator()
	{
		return Iterator(m_chunks);
	}


	uint32_t MTD_FLASHMEM getItemsCount()
	{
		uint32_t len = 0;
		Chunk* chunk = m_chunks;
		while (chunk)
		{
			len += chunk->items;
			chunk = chunk->next;
		}
		return len;
	}

private:
	Chunk* m_chunks;
	Chunk* m_current;
};



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// IterDict
// A dictionary where key and value are iterators

template <typename KeyIterator, typename ValueIterator>
class IterDict
{
public:

	struct Item
	{
		Item*         next;
		KeyIterator   key;
		ValueIterator value;
		
		Item(KeyIterator key_, ValueIterator value_)
			: next(NULL), key(key_), value(value_)
		{
		}
	};

	IterDict()
		: m_items(NULL), m_current(NULL), m_itemsCount(0)
	{
	}
	
	~IterDict()
	{
		clear();
	}
	
	void MTD_FLASHMEM clear()
	{
		Item* item = m_items;
		while (item)
		{
			Item* next = item->next;
			delete item;
			item = next;
		}
		m_items = m_current = NULL;
		m_itemsCount = 0;
	}
	
	void MTD_FLASHMEM add(KeyIterator key, ValueIterator value)
	{
		if (m_items)
		{
			m_current = m_current->next = new Item(key, value);
			++m_itemsCount;
		}
		else
		{
			m_current = m_items = new Item(key, value);
			++m_itemsCount;
		}
	}
	
	uint32_t MTD_FLASHMEM getItemsCount()
	{
		return m_itemsCount;
	}
	
	// warn: this doesn't check "index" range!
	Item& MTD_FLASHMEM getItem(uint32_t index)
	{
		Item* item = m_items;
		for (; index > 0; --index)
			item = item->next;
		return *item;
	}
	
	// warn: this doesn't check "index" range!
	Item& MTD_FLASHMEM operator[](uint32_t index)
	{
		return getItem(index);
	}
	
	// key stay in RAM or Flash
	ValueIterator MTD_FLASHMEM operator[](char const* key)
	{
		Item* item = m_items;
		while (item)
		{
			if (t_strcmp(item->key, CharIterator(key)) == 0)
				return item->value;
			item = item->next;
		}
		return ValueIterator();
	}
	
	// debug
	void dump()
	{
		for (uint32_t i = 0; i != m_itemsCount; ++i)
		{
			APtr<char> key(getItem(i).key.dup());
			APtr<char> value(getItem(i).value.dup());
			debug("%s = %s\r\n", key.get(), value.get());
		}
	}		
	
private:
	
	Item*    m_items;
	Item*    m_current;
	uint32_t m_itemsCount;
};



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// FlashDictionary
// Maximum one page (4096 bytes) of storage
// Maximum key length = 254 bytes
// Maximum value length = 65536 bytes (actually less than 4096!)
// All methods which accepts key and/or value allows Flash or RAM storage
//
// Examples:
//   FlashDictionary::setString("name", "Fabrizio");
//   debug("name = %s\r\n", FlashDictionary::getString("name", ""));
//
//   FlashDictionary::setInt("code", 1234);
//   debug("code = %d\r\n", FlashDictionary::getInt("name", 0));
//
// No need to call eraseContent the first time because it is automatically called if MAGIC is not found
// Call eraseContent only if you want to erase old dictionary

struct FlashDictionary
{

	static uint32_t const FLASH_DICTIONARY_POS = 0x14000;
	static uint32_t const MAGIC = 0x46445631;
	
	// clear the entire available space and write MAGIC at the beginning of the dictionary
	// This is required only if you want to remove previous content
	// erase all values to 0xFF
	static void MTD_FLASHMEM eraseContent()
	{
		Critical critical;
		spi_flash_erase_sector(FLASH_DICTIONARY_POS / SPI_FLASH_SEC_SIZE);
		uint32 magic = MAGIC;
		spi_flash_write(FLASH_DICTIONARY_POS, &magic, sizeof(magic));
	}

	// requires 4K free heap to execute!
	static void MTD_FLASHMEM setValue(char const* key, void const* value, uint32_t valueLength)
	{
		// find key or a free space
		uint8_t const* keyPosPtr = (uint8_t const*)findKey(key);

		// copy the Flash page in RAM buffer (page)
		// memcpy is ok, because source alredy aligned then can be read directly from Flash
		uint8_t* page = new uint8_t[SPI_FLASH_SEC_SIZE];
		memcpy(page, (void const*)(FLASH_MAP_START + FLASH_DICTIONARY_POS), SPI_FLASH_SEC_SIZE);	
		
		// get key position as index into page[]
		uint32_t keyPos = (uint32_t)keyPosPtr - FLASH_MAP_START - FLASH_DICTIONARY_POS;
		uint32_t curPos = keyPos;
		
		// is new key?
		bool isNewKey = (page[keyPos] == 0xFF);
		
		// store key length (byte)
		uint32_t keyLen = f_strlen(key);
		page[curPos++] = keyLen;
		
		// store key (and terminating zero)
		f_strcpy((char *)&page[curPos], key);
		curPos += keyLen + 1;

		// need to move next key->values?
		if (!isNewKey)
		{			
			uint32_t prevValueLength = page[curPos] | (page[curPos + 1] << 8);	// little-endian
			bool isLastKey = (page[curPos + 2 + prevValueLength] == 0xFF);
			int diff = (int32_t)valueLength - prevValueLength;
			uint8_t* dst = &page[curPos + max(0, diff)];
			uint8_t* src = &page[curPos - min(0, diff)];
			uint8_t* end = &page[SPI_FLASH_SEC_SIZE];
			memmove(dst, src, min(end - src, end - dst));
			// mark the new end of items
			if (isLastKey)
				page[curPos + 2 + valueLength] = 0xFF;			
		}
		
		// store low and high byte of value length word (little-endian)
		page[curPos++] = valueLength & 0xFF;
		page[curPos++] = (valueLength >> 8) & 0xFF;
		
		// store value data
		f_memcpy(&page[curPos], value, valueLength);
				
		// write back into the flash
		Critical critical;
		spi_flash_erase_sector(FLASH_DICTIONARY_POS / SPI_FLASH_SEC_SIZE);
		spi_flash_write(FLASH_DICTIONARY_POS, (uint32*)page, SPI_FLASH_SEC_SIZE);
		
		delete[] page;
	}
	
	// return NULL if key point to a free space (aka key doesn't exist)
	// return pointer may be Unaligned pointer to Flash
	static uint8_t const* MTD_FLASHMEM getValue(char const* key, uint32_t* valueLength = NULL)
	{
		uint8_t const* pos = (uint8_t const*)findKey(key);
		uint8_t keyLen = getByte(pos);
		if (keyLen == 0xFF)
			return NULL;
		pos += 1 + keyLen + 1;
		if (valueLength)
			*valueLength = getWord(pos);
		return pos + 2;
	}
		
	static void MTD_FLASHMEM setString(char const* key, char const* value)
	{
		setValue(key, value, f_strlen(value) + 1);
	}
	
	// always returns a Flash stored string (if not found return what contained in defaultValue)
	static char const* MTD_FLASHMEM getString(char const* key, char const* defaultValue)
	{
		char const* value = (char const*)getValue(key);
		return value? value : defaultValue;
	}
	
	static void MTD_FLASHMEM setInt(char const* key, int32_t value)
	{
		setValue(key, &value, sizeof(value));
	}
	
	static int32_t MTD_FLASHMEM getInt(char const* key, int32_t defaultValue)
	{
		void const* value = (void const*)getValue(key);
		return value? (int32_t)getDWord(value) : defaultValue;
	}
	
	static void MTD_FLASHMEM setBool(char const* key, bool value)
	{
		uint8_t b = value? 1 : 0;
		setValue(key, &b, 1);
	}
	
	static bool MTD_FLASHMEM getBool(char const* key, bool defaultValue)
	{
		void const* value = (void const*)getValue(key);
		return value? (getByte(value) == 1? true : false) : defaultValue;
	}
	
	// return starting of specified key or starting of next free position
	// what is stored:
	//   byte: key length (or 0xFF for end of keys, hence starting of free space). Doesn't include ending zero
	//   ....: key string
	//   0x00: key ending zero
	//   word: value length (little endian)
	//   ....: value data
	// automatically erase content if not already initialized
	static void const* MTD_FLASHMEM findKey(char const* key)
	{		
		if (!isContentValid())
			eraseContent();
		uint8_t const* curpos = (uint8_t const*)(FLASH_MAP_START + FLASH_DICTIONARY_POS + sizeof(MAGIC));	
		while (true)
		{
			uint8_t keylen = getByte(curpos);	// keylen doesn't include ending zero
			if (keylen == 0xFF || f_strcmp((char const*)(curpos + 1), key) == 0)
				return curpos;	// return start of free space or start of key->value block
			// goto next block
			curpos += 1 + keylen + 1;	// 1 (keylen field) + keylen + 1 (ending zero)
			curpos += 2 + getWord(curpos);	// 2 (valuelen field) + valuelen
		}
	}
	
	static bool MTD_FLASHMEM isContentValid()
	{
		// already aligned, can be read directly from Flash
		return *((uint32_t const*)(FLASH_MAP_START + FLASH_DICTIONARY_POS)) == MAGIC;
	}
	
};











}

#endif