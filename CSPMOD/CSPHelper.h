#pragma once

#include<SDL3/SDL.h>



#include<cryptopp890/rsa.h>
#include<cryptopp890/pssr.h>
#include<cryptopp890/sha.h>
#include<cryptopp890/osrng.h>


class CSPHelper
{
public:

	static void Init();


	static void OnClickEntrance();
	static inline void(*orig_OnClickEntrance)() = nullptr;




	static bool isProUser() { 

		uint32_t testCode[] = { 0x2e6f7250,0x747874 };
		SDL_PathInfo info;
		return SDL_GetPathInfo((const char*)testCode, &info);
	};


	//验证激活码//XXXXX-XXXXX-XXXXX
	static bool VerifySerial(const std::string& serial) {
	
		//处理序列号为随机数和签名
		std::string randstr = serial.substr(0,5)+ serial.substr(6,5);
		//CryptoPP::

	}



};