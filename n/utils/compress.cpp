/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "compress.h"
#include <n/core/Array.h>

namespace n {

constexpr uint MaxBits = 10;

struct ByteFreqs
{
	uint freqs[256];
};

struct HuffTableElem
{
	uint16 bits;
	byte nbBits;
};

struct HuffTable
{
	HuffTableElem elems[256];
};

struct HuffReverseElem
{
	byte b;
	byte nbBits;
};

struct HuffReverse
{
	HuffReverseElem elems[1 << MaxBits];
};

struct HuffNode
{
	uint f;
	byte b;
	HuffNode *left;
	HuffNode *right;
};

ByteFreqs countBytes(const byte *data, uint size) {
	uint counts[4][256] = {{0}};
	uint cSize = size / 4;
	uint i = 0;
	for(; i != cSize; i++) {
		uint b = i * 4;
		counts[0][data[b]]++;
		counts[1][data[b + 1]]++;
		counts[2][data[b + 2]]++;
		counts[3][data[b + 3]]++;
	}
	for(i *= 4; i != size; i++) {
		counts[0][data[i]]++;
	}
	ByteFreqs fr;
	for(i = 0; i != 256; i++) {
		fr.freqs[i] = counts[0][i] + counts[1][i] + counts[2][i] + counts[3][i];
	}
	return fr;
	/*ByteFreqs fr = {0};
	for(uint i = 0; i != size; i++) {
		fr.freqs[data[i]]++;
	}
	return fr;*/
}

HuffNode *buildHuffTree(ByteFreqs freqs) {
	core::Array<HuffNode *> nodes;
	for(uint i = 0; i != 256; i++) {
		if(freqs.freqs[i]) {
			nodes.append(new HuffNode{freqs.freqs[i], byte(i), 0, 0});
		}
	}
	while(nodes.size() > 1) {
		nodes.sort([](HuffNode *a, HuffNode *b) {
			return a->f > b->f;
		});
		HuffNode *n1 = nodes.last();
		nodes.pop();
		HuffNode *n2 = nodes.last();
		nodes.pop();
		nodes.append(new HuffNode{n1->f + n2->f, 0, n1, n2});
	}
	return nodes.last();
}

void buildHuffTableRec(HuffTable &table, HuffNode *node, HuffTableElem elem) {
	if(!node) {
		return;
	}
	if(node->left || node->right) {
		HuffTableElem l{uint16(elem.bits << 1), byte(elem.nbBits + 1)};
		HuffTableElem r{uint16((elem.bits << 1) | 0x01), byte(elem.nbBits + 1)};
		buildHuffTableRec(table, node->left, l);
		buildHuffTableRec(table, node->right, r);
	} else {
		if(elem.nbBits > MaxBits) {
			fatal("Maximum per-byte bits reached.");
		}
		table.elems[node->b] = elem;
	}
}

HuffTable buildHuffTable(HuffNode *node) {
	HuffTable table = {{0, 0}};
	buildHuffTableRec(table, node, HuffTableElem{0, 0});
	return table;
}

void huffCompress(HuffTable t, byte *out, const byte *data, uint size) {
	uint32 buffer = 0;
	uint bufferBits = 0;
	const byte *end = data + size;
	for(const byte *it = data; it != end; it++) {
		HuffTableElem e = t.elems[*it];
		buffer <<= e.nbBits;
		buffer |= e.bits;
		bufferBits += e.nbBits;
		while(bufferBits >= 8) {
			*out = (buffer >> (bufferBits - 8)) & 0x00FF;
			bufferBits -= 8;
			out++;
		}
	}
	if(bufferBits) {
		buffer <<= 8 - bufferBits;
		*out = byte(buffer & 0xFF);
	}
}

uint compressedHuffSize(HuffTable t, ByteFreqs f) {
	uint64 bits = 0;
	for(uint i = 0; i != 256; i++) {
		bits += uint64(t.elems[i].nbBits) * uint64(f.freqs[i]);
	}
	uint64 bytes = bits / 8;
	if(bits % 8) {
		bytes++;
	}
	return bytes;
}

uint huffTableSize(HuffTable t) {
	uint si = 0;
	for(uint i = 0; i != 256; i++) {
		if(t.elems[i].nbBits) {
			si++;
		}
	}
	return si;
}

void writeHuffFreqs(ByteFreqs t, byte *out)  {
	for(uint i = 0; i != 256; i++) {
		if(t.freqs[i]) {
			*out = byte(i);
			out++;
			*reinterpret_cast<uint32 *>(out) = t.freqs[i];
			out += 4;
		}
	}
}

ByteFreqs readHuffFreqs(const byte *data) {
	uint tSize = *reinterpret_cast<const uint16 *>(data);
	data += 2;
	ByteFreqs freqs = {{0}};
	for(uint i = 0; i != tSize; i++) {
		byte e = *data;
		data++;
		freqs.freqs[e] = *reinterpret_cast<const uint32 *>(data);
		data += 4;
	}
	return freqs;
}

HuffReverse buildReverseHuff(HuffTable t) {
	HuffReverse rev {{0, 0}};
	for(uint i = 0; i != 256; i++) {
		if(t.elems[i].nbBits) {
			uint nb = t.elems[i].nbBits;
			uint b = t.elems[i].bits;
			for(uint j = 0; j != 1 << MaxBits; j++) {
				if((j >> (MaxBits - nb)) == b) {
					rev.elems[j].b = i;
					rev.elems[j].nbBits = nb;
				}
			}
		}
	}
	return rev;
}

void huffUncompress(HuffReverse rev, const byte *data, byte *out, uint size) {
	uint32 buffer = 0;
	uint bufferBits = 0;
	byte *end = out + size;
	uint32 mask = uint32(0xFFFFFFFFF);
	mask = ~(mask << MaxBits);
	while(out != end) {
		while(bufferBits < MaxBits) {
			buffer <<= 8;
			buffer |= *data;
			data++;
			bufferBits += 8;
		}
		uint32 index = (buffer >> (bufferBits - MaxBits)) & mask;
		HuffReverseElem e = rev.elems[index];
		*out = e.b;
		out++;
		bufferBits -= e.nbBits;
	}
}


void *compress(const void *data, uint size, uint *compressedSize) {
	const byte *bytes = reinterpret_cast<const byte *>(data);
	ByteFreqs freqs = countBytes(bytes, size);
	HuffNode *node = buildHuffTree(freqs);
	HuffTable table = buildHuffTable(node);

	/*for(uint i = 0; i != 256; i++) {
		if(table.elems[i].nbBits) {
			std::cout<<i<<" = ("<<uint(table.elems[i].nbBits)<<") "<<uint(table.elems[i].bits)<<std::endl;
		}
	}*/

	uint tSize = huffTableSize(table);
	uint cSize = compressedHuffSize(table, freqs) + tSize * 5 + 2;
	if(compressedSize) {
		*compressedSize = cSize;
	}
	byte *cData = new byte[cSize];
	*reinterpret_cast<uint16 *>(cData) = uint16(tSize);
	writeHuffFreqs(freqs, cData + 2);
	huffCompress(table, cData + tSize * 5 + 2, bytes, size);

	return cData;
}


void *uncompress(const void *data, uint *uncompressedSize) {
	const byte *bytes = reinterpret_cast<const byte *>(data);
	uint tSize = *reinterpret_cast<const uint16 *>(bytes);
	ByteFreqs freqs = readHuffFreqs(bytes);
	bytes += 2 + tSize * 5;
	HuffNode *node = buildHuffTree(freqs);
	HuffTable table = buildHuffTable(node);

	uint uSize = 0;
	for(uint i = 0; i != 256; i++) {
		uSize += freqs.freqs[i];
	}

	if(uncompressedSize) {
		*uncompressedSize = uSize;
	}

	/*for(uint i = 0; i != 256; i++) {
		if(table.elems[i].nbBits) {
			std::cout<<i<<" = ("<<uint(table.elems[i].nbBits)<<") "<<uint(table.elems[i].bits)<<std::endl;
		}
	}*/

	HuffReverse rev = buildReverseHuff(table);
	byte *uData = new byte[uSize];
	huffUncompress(rev, bytes, uData, uSize);
	return uData;
}



}
