//-----------------------------------------------------------------------------
//
//	bitfield.h
//
//	Integer to Bits Helper Class
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "Bitfield.h"

namespace OpenZWave
{
	namespace Internal
	{
		/* Note: we have our own "BitSet" class rather than using the Std Library version
		 * as the Std Library version can not be resized at runtime. This version can. Its the basis
		 * of the BitSet ValueID class
		 */

		Bitfield::Bitfield() :
				m_numSetBits(0), m_value(0)
		{
		}

		Bitfield::Bitfield(uint32 value) :
				m_numSetBits(0), m_value(value)
		{
			for (unsigned int i = 0; i < 8 * sizeof(uint32); i++)
			{
				if (m_value & (1 << i))
				{
					Set(i);
				}
			}
		}
		Bitfield::~Bitfield()
		{

		}

		bool Bitfield::SetValue(uint32 val)
		{
			for (unsigned int i = 0; i < 8 * sizeof(uint32); i++)
			{
				if (val & (1 << i))
				{
					Set(i);
				}
				else
				{
					Clear(i);
				}
			}
			return true;
		}

		bool Bitfield::Set(uint8 _idx)
		{
			if (_idx > 0x1F)
			{
				return false;
			}

			if (!IsSet(_idx))
			{
				uint32 newSize = (_idx >> 5) + 1;
				if (newSize > m_bits.size())
				{
					m_bits.resize(newSize, 0);
				}
				m_bits[_idx >> 5] |= (1 << (_idx & 0x1f));
				++m_numSetBits;
			}
			return true;
		}

		bool Bitfield::Clear(uint8 _idx)
		{
			if (_idx > 0x1F)
			{
				return false;
			}
			if (IsSet(_idx))
			{
				m_bits[_idx >> 5] &= ~(1 << (_idx & 0x1f));
				--m_numSetBits;
			}
			return true;
		}

		bool Bitfield::IsSet(uint8 _idx) const
		{
			if (_idx > 0x1F)
			{
				return false;
			}
			if ((unsigned int) (_idx >> 5) < (unsigned int) m_bits.size())
			{
				return ((m_bits[_idx >> 5] & (1 << (_idx & 0x1f))) != 0);
			}
			return false;
		}

		uint32 Bitfield::GetNumSetBits() const
		{
			return m_numSetBits;
		}

		uint32 Bitfield::GetValue() const
		{
			uint32 value = 0;
			for (unsigned int i = 0; i < m_bits.size(); i++)
			{
				value += (m_bits[i] << (8 * i));
			}
			return value;
		}

		uint32 Bitfield::GetSize() const
		{
			return m_bits.size() * sizeof(uint32) * 8;
		}

		Bitfield::Iterator Bitfield::Begin() const
		{
			return Iterator(this, 0);
		}
		Bitfield::Iterator Bitfield::End() const
		{
			return Iterator(this, (uint32) m_bits.size() << 5);
		}

		uint32 Bitfield::Iterator::operator *() const
		{
			return m_idx;
		}

		Bitfield::Iterator& Bitfield::Iterator::operator++()
		{
			// Search forward to next set bit
			NextSetBit();
			return *this;
		}

		Bitfield::Iterator Bitfield::Iterator::operator++(int a)
		{
			Iterator tmp = *this;
			++tmp;
			return tmp;
		}

		bool Bitfield::Iterator::operator ==(const Iterator &rhs)
		{
			return m_idx == rhs.m_idx;
		}

		bool Bitfield::Iterator::operator !=(const Iterator &rhs)
		{
			return m_idx != rhs.m_idx;
		}

		Bitfield::Iterator::Iterator(Bitfield const* _bitfield, uint32 _idx) :
				m_idx(_idx), m_bitfield(_bitfield)
		{
			// If this is a begin iterator, move it to the first set bit
			if ((_idx == 0) && (!m_bitfield->IsSet(0)))
			{
				NextSetBit();
			}
		}

		void Bitfield::Iterator::NextSetBit()
		{
			while (((++m_idx) >> 5) < m_bitfield->m_bits.size())
			{
				// See if there are any bits left to find in the current uint32
				if ((m_bitfield->m_bits[m_idx >> 5] & ~((1 << (m_idx & 0x1f)) - 1)) == 0)
				{
					// No more bits - move on to next uint32 (or rather one less than
					// the next uint32 because of the preincrement in the while statement)
					m_idx = (m_idx & 0xffffffe0) + 31;
				}
				else
				{
					if ((m_bitfield->m_bits[m_idx >> 5] & (1 << (m_idx & 0x1f))) != 0)
					{
						// This bit is set
						return;
					}
				}
			}
		}
	} // namespace Internal
} // namespace OpenZWave
