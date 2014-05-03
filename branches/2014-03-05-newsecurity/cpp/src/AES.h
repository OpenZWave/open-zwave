//-----------------------------------------------------------------------------
//
//	AES.h
//
//	Implementation of 128-bit AES Encryption
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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

#ifndef _AES_H
#define _AES_H

#include "Defs.h"

namespace OpenZWave
{
	class AES
	{
	public:
		typedef enum AESMode {
			AESMode_CFB,
			AESMode_OFB,
			AESMode_CBC
		} AESMode;

		AES();
		virtual ~AES();

		// Expand a user-supplied key material into a session key.
		// key        - The 128 bit user-key to use.
		// chain      - initial chain block for CBC and CFB modes.
		void MakeKey( char const* key, char const* chain );

	private:
		//Auxiliary Function
		void Xor( char* buff, char const* chain )
		{
			for(int32 i=0; i<16; i++)
			{
				*(buff++) ^= *(chain++);
			}
		}


	public:
		// Encrypt exactly one 16 byte block of plaintext
		// in         - The plaintext
		// result     - The ciphertext generated from a plaintext using the key
		void EncryptBlock( char const* in, char* result );

		// Decrypt exactly one 16 bytes block of plaintext
		// in         - The ciphertext.
		// result     - The plaintext generated from a ciphertext using the session key.
		void DecryptBlock( char const* in, char* result );


		bool Encrypt( char const* _in, char* _result, int32 _size, AESMode _mode);
		bool Decrypt( char const* _in, char* _result, int32 _size, AESMode _mode);


		//Number of Rounds
		int32 GetRounds()
		{
			return m_rounds;
		}

		void ResetChain()
		{
			memcpy( m_chain, m_chain0, 16 );
		}

	public:
		//Null chain
		static char const* s_chain0;

	private:
		enum
		{ 
			MAX_ROUNDS=14,
			MAX_KC=8,
			MAX_BC=8
		};

		// Helper  Functions
		// Multiply two elements of GF(2^m)
		static int32 Mul(int32 a, int32 b)
		{
			return (a != 0 && b != 0) ? s_alog[(s_log[a & 0xff] + s_log[b & 0xff]) % 255] : 0;
		}

		// Helper method used in generating Transposition Boxes
		static int32 Mul4(int32 a, char b[])
		{
			if(a == 0)
			{
				return 0;
			}

			a = s_log[a & 0xff];
			int32 a0 = (b[0] != 0) ? s_alog[(a + s_log[b[0] & 0xff]) % 255] & 0xff : 0;
			int32 a1 = (b[1] != 0) ? s_alog[(a + s_log[b[1] & 0xff]) % 255] & 0xff : 0;
			int32 a2 = (b[2] != 0) ? s_alog[(a + s_log[b[2] & 0xff]) % 255] & 0xff : 0;
			int32 a3 = (b[3] != 0) ? s_alog[(a + s_log[b[3] & 0xff]) % 255] & 0xff : 0;
			
			return( (a0<<24) | (a1<<16) | (a2<<8) | a3 );
		}


		static const int32	s_alog[256];
		static const int32	s_log[256];
		static const int8	s_S[256];
		static const int8	s_Si[256];
		static const int32	s_T1[256];
		static const int32	s_T2[256];
		static const int32	s_T3[256];
		static const int32	s_T4[256];
		static const int32	s_T5[256];
		static const int32	s_T6[256];
		static const int32	s_T7[256];
		static const int32	s_T8[256];
		static const int32	s_U1[256];
		static const int32	s_U2[256];
		static const int32	s_U3[256];
		static const int32	s_U4[256];
		static const int8	s_rcon[30];
		static const int32	s_shifts[3][4][2];

		//Key Initialization Flag
		bool m_bKeyInit;
		
		int32 m_Ke[MAX_ROUNDS+1][MAX_BC];
		int32 m_Kd[MAX_ROUNDS+1][MAX_BC];
		int32 m_rounds;
		
		//Chain Block
		char m_chain0[16];
		char m_chain[16];
		
		// Private use buffers
		int32 tk[MAX_KC];
//		int32 a[MAX_BC];
//		int32 t[MAX_BC];
	};

} // Namespace OpenZWave

#endif // _AES_H

