#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <peekpoke.h>

#define RESOLUTION_X 160
#define RESOLUTION_Y 200
#define ORIGIN_Z 0
#define MYCOLORS 0xFC	// color 2, color 1
#define MYCOLORS2 0x01	// unused, color 3
#define vicBankAdress 0x4000
#define bitmapAdress 0x6000

// STRUCTS

// Floating points needs 5 bytes in RAM
struct floatingPoint {
	char exponent;
	char mantissa1;
	char mantissa2;
	char mantissa3;
	char mantissa4;
};

// vector3 needs 3*5 = 15 bytes in RAM
struct vector3 {
	struct floatingPoint x; // 5 bytes each
	struct floatingPoint y;
	struct floatingPoint z;
};

//GLOBAL VARIABLES
unsigned int adress;
signed int integer;
unsigned char charact;
unsigned char adressLow;
unsigned char adressHigh;
unsigned char numberLow;
unsigned char numberHigh;
struct floatingPoint tempFPx;
struct floatingPoint tempFPy;
struct floatingPoint tempFPz;
struct floatingPoint half;



// FLOATING POINT MATH - using Basic and Kernal ROM routines called from assembly
// Made by Marius Irgens, but based on this awesome documentation:
// https://codebase64.org/doku.php?id=base:kernal_floating_point_mathematics

//multipurpose adress-to-register load (A/Y low/high)
void LoadRegAYwithAdress(unsigned int adress) {
	adressLow = (unsigned char)adress;
	adressHigh = (unsigned char)(adress >> 8);
	__asm__("lda %v", adressLow);
	__asm__("ldy %v", adressHigh);
}

//Load FAC1 with whole number using "integer to FP" routine
void loadFAC1Immediate(signed int number) {
	numberLow = (unsigned char)number;
	numberHigh = (unsigned char)(number >> 8);
	__asm__("ldy %v", numberLow);
	__asm__("lda %v", numberHigh);
	__asm__("jsr $b391"); //integer to FP
}

//Store number in FAC1 into memory adress
void storeFAC1InMem(unsigned int adress) {
	adressLow = (unsigned char)adress;
	adressHigh = (unsigned char)(adress >> 8);
	__asm__("ldx %v", adressLow);
	__asm__("ldy %v", adressHigh);
	__asm__("jsr $bbd4"); //store FP in memory
}

//Load FAC1 from memory adress
void loadFAC1fromMem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $bba2"); // Load FP from memory
}

//Load FAC2 from memory adress
void loadFAC2fromMem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $ba8c"); //Load FP from memory
}

//Move contents of FAC1 into FAC2 (faster than loading FAC2 from memory)
void moveFAC1toFAC2() {
	__asm__("jsr $bc0f"); //store FP in memory
}

//Print FAC1 
void printFAC1() {
	__asm__("jsr $bddd"); //FP to string at 0x0100
	cprintf("%s ", 0x0100);
}

void printFP(unsigned int adress) {
	loadFAC1fromMem(adress);
	printFAC1();
}

void addFACs() {
	__asm__("lda $61"); //to set zero flag
	__asm__("jsr $b86a"); //Addition - FAC1 and FAC2
}

void addFAC1Mem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $b867"); //Addition - FAC1 and value at mem adress
}

void subtractFACs() {
	__asm__("jsr $b853"); //subtract - FAC1 from FAC2 (FAC2-FAC1)
}

void subtractFAC1Mem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $b850"); //subtract - FAC1 from value at mem adress
}

void multiplyFACs() {
	__asm__("lda $61"); //to set zero flag
	__asm__("jsr $ba2b"); //multiplication - FAC1 and FAC2
}

void multiplyFAC1Mem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $ba28"); //multiplication - FAC1 and value at mem adress
}
//
//void divideFACs() {
//	__asm__("lda $61"); //to set zero flag (sign comparison not performed)
//	__asm__("jsr $bb12"); //division - FAC2 by FAC1 - quot in FAC1, rem in FAC2 -(FAC2/FAC1)
//}

void divideFAC1Mem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $bb0f"); //division -  value at mem adress by FAC1
}

void multiplyFAC1by10() {
	__asm__("jsr $bae2");
}

void divideFAC1by10() {
	__asm__("jsr $bafe");
}

void squaredOfFAC1() {
	__asm__("jsr $bf71");
}

void powFAC2toMem(unsigned int adress) {
	loadFAC1fromMem(adress);
	__asm__("jsr $bf7b"); //FAC2^FAC1
}

void powFAC2toFAC1() {
	__asm__("jsr $bf7b"); //FAC2^FAC1
}

//void evaluateSignFAC1(unsigned int destAdress) {
//	__asm__("jsr $bc39"); // $01 = pos, $ff = neg, $00 = 0;
//	charact = PEEK(0x0065);
//	POKE(destAdress, charact);
//}

//void compareFAC1toMem(unsigned int adress, unsigned int destAdress) {
//	LoadRegAYwithAdress(adress);
//	__asm__("jsr $bc5b");
//	__asm__("sta %i", destAdress);
//}

void FAC1toInt(unsigned int destAdress) {
	__asm__("jsr $bc9b"); // Truncate FAC1
	numberLow = PEEK(0x0065);
	numberHigh = PEEK(0x0064);
	integer = 0x00ff & numberHigh;
	integer = integer << 8;
	integer += numberLow;
	POKEW(destAdress, integer);

}

void makeFPImmediate(signed int value, unsigned int destAdress) {
	loadFAC1Immediate(value);
	storeFAC1InMem(destAdress);
}

void fillVectorValues(unsigned int vecAdress, signed int x, signed int y, signed int z) {
	loadFAC1Immediate(x);
	storeFAC1InMem(vecAdress);
	loadFAC1Immediate(y);
	storeFAC1InMem(vecAdress + 5);
	loadFAC1Immediate(z);
	storeFAC1InMem(vecAdress + 10);
}

void printVectorValues(unsigned int vecAdress) {
	cprintf("Vec at  %x - x: ", vecAdress);
	printFP(vecAdress);
	cprintf(" , y: ");
	printFP(vecAdress + 5);
	cprintf(" , z: ");
	printFP(vecAdress + 10);
	//cprintf("\n \r");
}

void vectorAddition(unsigned int vecAdressA, unsigned int vecAdressB, unsigned int destAdress) {
	//Add B.x to A.x and store it in dest.x (A-B)
	loadFAC1fromMem(vecAdressA);
	addFAC1Mem(vecAdressB);
	storeFAC1InMem(destAdress);

	//subtract B.y from A.y and store it in dest.y (A-B)
	loadFAC1fromMem(vecAdressA + 5);
	addFAC1Mem(vecAdressB + 5);
	storeFAC1InMem(destAdress + 5);

	//subtract B.z from A.z and store it in dest.z (A-B)
	loadFAC1fromMem(vecAdressA + 10);
	addFAC1Mem(vecAdressB + 10);
	storeFAC1InMem(destAdress + 10);
}

void vectorSubtraction(unsigned int vecAdressA, unsigned int vecAdressB, unsigned int destAdress) {
	//subtract B.x from A.x and store it in dest.x (A-B)
	loadFAC1fromMem(vecAdressB);
	subtractFAC1Mem(vecAdressA);
	storeFAC1InMem(destAdress);

	//subtract B.y from A.y and store it in dest.y (A-B)
	loadFAC1fromMem(vecAdressB + 5);
	subtractFAC1Mem(vecAdressA + 5);
	storeFAC1InMem(destAdress + 5);

	//subtract B.z from A.z and store it in dest.z (A-B)
	loadFAC1fromMem(vecAdressB + 10);
	subtractFAC1Mem(vecAdressA + 10);
	storeFAC1InMem(destAdress + 10);
}

void dotProduct(unsigned int vecAdressA, unsigned int vecAdressB, unsigned int destAdress) {
	// multiply A.x and B.x and store in tempFPx 
	loadFAC1fromMem(vecAdressA);
	multiplyFAC1Mem(vecAdressB);
	storeFAC1InMem(&tempFPx);
	// multiply A.y and B.y and store in tempFPy 
	loadFAC1fromMem(vecAdressA + 5);
	multiplyFAC1Mem(vecAdressB + 5);
	storeFAC1InMem(&tempFPy);
	// multiply A.z and B.z and let stay in FAC1 
	loadFAC1fromMem(vecAdressA + 10);
	multiplyFAC1Mem(vecAdressB + 10);
	// add tempFPx and tempFPy to tempFPz
	addFAC1Mem(&tempFPx);
	addFAC1Mem(&tempFPy);
	// store in destination
	storeFAC1InMem(destAdress);
}

void vectorLength(unsigned int vecAdressA, unsigned int destAdress) {
	//calculate x^2 and put it in tempFPx
	loadFAC2fromMem(vecAdressA);
	loadFAC1Immediate(2);
	powFAC2toFAC1();
	storeFAC1InMem(&tempFPx);

	//calculate y^2 and put it in tempFPy
	loadFAC2fromMem(vecAdressA + 5);
	loadFAC1Immediate(2);
	powFAC2toFAC1();
	storeFAC1InMem(&tempFPy);

	//calculate z^2 and put it in tempFPz
	loadFAC2fromMem(vecAdressA + 10);
	loadFAC1Immediate(2);
	powFAC2toFAC1();
	storeFAC1InMem(&tempFPz);

	// add tempFPx and tempFPy to tempFPz
	addFAC1Mem(&tempFPx);
	addFAC1Mem(&tempFPy);

	// take square root of sum of (x^2 + y^2 + z^2)
	squaredOfFAC1();

	// store at destination adress
	storeFAC1InMem(destAdress);
}

void normalizeVector(unsigned int vecAdressA, unsigned int destAdress) {
	// calculate vector length and store in tempFPx
	vectorLength(vecAdressA, &tempFPx);

	// divide x by vector length and store in dest.x 
	loadFAC1fromMem(&tempFPx); // length
	divideFAC1Mem(vecAdressA); // divide A.x by length
	storeFAC1InMem(destAdress); //store in dest.x

	// divide y by vector length and store in dest.y 
	loadFAC1fromMem(&tempFPx); // length
	divideFAC1Mem(vecAdressA + 5); // divide A.y by length
	storeFAC1InMem(destAdress + 5); //store in dest.y

	// divide z by vector length and store in dest.z 
	loadFAC1fromMem(&tempFPx); // length
	divideFAC1Mem(vecAdressA + 10); // divide A.z by length
	storeFAC1InMem(destAdress + 10); //store in dest.z
}

void makeFraction(signed int x, signed int y, unsigned int destAdress) {
	loadFAC1Immediate(x);
	storeFAC1InMem(&tempFPx);
	loadFAC1Immediate(y);
	divideFAC1Mem(&tempFPx);			// (mem / FAC1)
	storeFAC1InMem(destAdress);
}

void calcOrigin(unsigned int x, unsigned int y, unsigned int destAdress) {
	//origin.x
	// x / resolution
	loadFAC1Immediate(x);
	storeFAC1InMem(&tempFPx);
	loadFAC1Immediate(RESOLUTION_X);
	divideFAC1Mem(&tempFPx);			// (mem / FAC1)
	// (x / resolution) - 0.5
	moveFAC1toFAC2();
	loadFAC1fromMem(&half);
	subtractFACs();						// (FAC2 - FAC1)
	// ((x / resolution) -0.5) * 2
	storeFAC1InMem(&tempFPx);
	loadFAC1Immediate(2);
	multiplyFAC1Mem(&tempFPx);
	storeFAC1InMem(destAdress);

	//origin.y
	// y / resolution
	loadFAC1Immediate(y);
	storeFAC1InMem(&tempFPy);
	loadFAC1Immediate(RESOLUTION_Y);
	divideFAC1Mem(&tempFPy);			// (mem / FAC1)
	// (y / resolution) - 0.5
	moveFAC1toFAC2();
	loadFAC1fromMem(&half);
	subtractFACs();
	// ((y / resolution) -0.5) * -2		// -2 = flip image
	storeFAC1InMem(&tempFPy);
	loadFAC1Immediate(-2);
	multiplyFAC1Mem(&tempFPy);
	storeFAC1InMem(destAdress + 5);

	loadFAC1Immediate(ORIGIN_Z);
	storeFAC1InMem(destAdress + 10);
}

void sphereIntersect(unsigned int ro, unsigned int rd, unsigned int sc, unsigned int sr, unsigned int destAdress) {
	//ro = ray origin
	//rd = ray direction
	//sc = sphere center
	//sr = sphere radius
	//destAdress = destination floating point (answer - t value)
	struct floatingPoint a;
	struct floatingPoint b;
	struct floatingPoint c;
	struct floatingPoint D;
	struct floatingPoint tempFPv;
	signed int ifCheck;

	//printVectorValues(ro);

	// a = dot(rayDirection, rayDirection)
	dotProduct(rd, rd, &a);


	// b = 2*dot(rayOrigin, rayDirection) - 2*dot(rayDirection, sphereCenter)
	// (rayOrigin, rayDirection) into tempFPx
	dotProduct(ro, rd, &tempFPx);

	// 2*dot(rayOrigin, rayDirection)
	loadFAC1Immediate(2);
	loadFAC2fromMem(&tempFPx);
	multiplyFACs();
	storeFAC1InMem(&tempFPx);

	// (rayDirection, sphereCenter) into tempFPy
	dotProduct(rd, sc, &tempFPy);

	// 2*dot(rayDirection, sphereCenter)
	loadFAC1Immediate(2);
	loadFAC2fromMem(&tempFPy);
	multiplyFACs();
	storeFAC1InMem(&tempFPy);

	// 2*dot(rayOrigin, rayDirection) - 2*dot(rayDirection, sphereCenter)
	// FAC2	- FAC1
	loadFAC2fromMem(&tempFPx);
	loadFAC1fromMem(&tempFPy);
	subtractFACs();
	storeFAC1InMem(&b);


	// c = dot(rayOrigin, rayOrigin) -2*dot(rayOrigin, sphereCenter) + dot(sphereCenter, sphereCenter) - sphereRadius^2
	// dot(rayOrigin, rayOrigin)
	dotProduct(ro, ro, &tempFPx);

	// dot(rayOrigin, sphereCenter)
	dotProduct(ro, sc, &tempFPy);
	// 2*dot(rayOrigin, sphereCenter)
	loadFAC1Immediate(2);
	loadFAC2fromMem(&tempFPy);
	multiplyFACs();
	storeFAC1InMem(&tempFPy);

	// dot(sphereCenter, sphereCenter)
	dotProduct(sc, sc, &tempFPz);

	// sphereRadius ^ 2
	loadFAC2fromMem(sr);
	loadFAC1Immediate(2);
	powFAC2toFAC1();
	storeFAC1InMem(&tempFPv);

	// dot(rayOrigin, rayOrigin) -2*dot(rayOrigin, sphereCenter) - 1st half
	// FAC2	- FAC1
	loadFAC2fromMem(&tempFPx);
	loadFAC1fromMem(&tempFPy);
	subtractFACs();
	storeFAC1InMem(&tempFPx);

	// dot(sphereCenter, sphereCenter) - sphereRadius^2 - 2nd half
	// FAC2	- FAC1
	loadFAC2fromMem(&tempFPz);
	loadFAC1fromMem(&tempFPv);
	subtractFACs();
	storeFAC1InMem(&tempFPy);

	// dot(rayOrigin, rayOrigin) -2*dot(rayOrigin, sphereCenter) + dot(sphereCenter, sphereCenter) - sphereRadius^2
	loadFAC1fromMem(&tempFPx);
	loadFAC2fromMem(&tempFPy);
	addFACs();
	storeFAC1InMem(&c);


	// D = pow(b, 2) - 4*a*c
	// pow(b, 2)
	loadFAC2fromMem(&b);
	loadFAC1Immediate(2);
	powFAC2toFAC1();
	storeFAC1InMem(&tempFPx);

	// 4*a*c
	loadFAC1fromMem(&a);
	loadFAC2fromMem(&c);
	multiplyFACs();
	moveFAC1toFAC2();
	loadFAC1Immediate(4);
	multiplyFACs();
	storeFAC1InMem(&tempFPy);

	// pow(b, 2) - 4*a*c
	// FAC2	- FAC1
	loadFAC2fromMem(&tempFPx);
	loadFAC1fromMem(&tempFPy);
	subtractFACs();
	storeFAC1InMem(&D);

	// 	if (D >= 0)
	//evaluateSignFAC1(); // $01 = pos, $ff = neg, $00 = 0;
	loadFAC1fromMem(&D);
	FAC1toInt(&ifCheck);
	if (ifCheck >= 0) {
		//cprintf("%i", ifCheck);

		//ret = (-b - sqrt(D)) / 2 * a  - NEAREST HIT
		//-b
		loadFAC1Immediate(0);
		moveFAC1toFAC2();
		loadFAC1fromMem(&b);
		subtractFACs();
		storeFAC1InMem(&tempFPx);

		//sqrt(D)
		loadFAC1fromMem(&D);
		squaredOfFAC1();
		storeFAC1InMem(&tempFPy);

		// 2 * a
		loadFAC2fromMem(&a);
		loadFAC1Immediate(2);
		multiplyFACs();
		storeFAC1InMem(&tempFPv);

		//(-b - sqrt(D))
		loadFAC2fromMem(&tempFPx);
		loadFAC1fromMem(&tempFPy);
		subtractFACs();
		storeFAC1InMem(&tempFPz);

		//(-b - sqrt(D)) / 2 * a (FAC2/FAC1)
		/*loadFAC2fromMem(&tempFPz);
		loadFAC1fromMem(&tempFPv);
		divideFACs();*/
		//(mem/FAC1)
		loadFAC1fromMem(&tempFPv);
		divideFAC1Mem(&tempFPz);
		storeFAC1InMem(destAdress);
	}
	else {
		loadFAC1Immediate(-1);
		storeFAC1InMem(destAdress);
	}
}

char getColorValue(int x, int y) {
	int cx = 80;
	int cy = 100;
	int vecLength;

	int vecx = abs(x - cx) * 2;
	int vecy = abs(y - cy);

	vecLength = (vecx * vecx) + (vecy * vecy);
	//vecLength = mySqrt(vecLength);

	if (vecLength <= 16000) {
		return 1;
	}
	else {
		return 0;
	}
}

//Draw pixel while using multicolor bitmap mode
void drawPixelMBM(unsigned int x, unsigned int y, unsigned char color) {
	unsigned char addedPixel;
	unsigned char existingPixel;
	unsigned int memAddr;
	div_t xDiv;
	div_t yDiv;
	xDiv = div(x, 4);
	yDiv = div(y, 8);

	if (xDiv.rem == 0 && color == 1) {
		addedPixel = 0b10000000;
	}
	else if (xDiv.rem == 0 && color == 2) {
		addedPixel = 0b01000000;
	}
	else if (xDiv.rem == 0 && color == 3) {
		addedPixel = 0b11000000;
	}
	else if (xDiv.rem == 1 && color == 1) {
		addedPixel = 0b00100000;
	}
	else if (xDiv.rem == 1 && color == 2) {
		addedPixel = 0b00010000;
	}
	else if (xDiv.rem == 1 && color == 3) {
		addedPixel = 0b00110000;
	}
	else if (xDiv.rem == 2 && color == 1) {
		addedPixel = 0b00001000;
	}
	else if (xDiv.rem == 2 && color == 2) {
		addedPixel = 0b00000100;
	}
	else if (xDiv.rem == 2 && color == 3) {
		addedPixel = 0b00001100;
	}
	else if (xDiv.rem == 3 && color == 1) {
		addedPixel = 0b00000010;
	}
	else if (xDiv.rem == 3 && color == 2) {
		addedPixel = 0b00000001;
	}
	else if (xDiv.rem == 3 && color == 3) {
		addedPixel = 0b00000011;
	}
	memAddr = bitmapAdress + (yDiv.quot * 320) + (xDiv.quot * 8) + (yDiv.rem);
	existingPixel = PEEK(memAddr);
	addedPixel = existingPixel + addedPixel;
	POKE(memAddr, addedPixel);
	//Scputcxy(xDiv.quot, yDiv.quot, color + MYCOLORS);
}

//void FloatMathDebug() {
//	struct vector3 rayOrigin;
//	struct vector3 projPoint;
//	struct vector3 rayDirection;
//	struct vector3 sphereCenter;
//	struct floatingPoint sphereRadius;
//	struct floatingPoint tValue;
//	signed int tValueInt;
//	//signed char ifCheck;
//
//	//SETUP
//	unsigned int memAddr = 0x2000;	// adress at start of bitmap memory RAM location
//	POKE(0x0001, 0b00110111);		//ram Configuration
//
//	makeFraction(1, 2, &half);
//
//	//set projection angle point (change Z position to change lens angle)
//	fillVectorValues(&projPoint, 0, 0, -1);
//
//	//set Sphere center and radius
//	fillVectorValues(&sphereCenter, 0, 0, 5);
//	makeFPImmediate(2, &sphereRadius);
//
//	//LOOP
//
//	//Calculate origin vector at pixel
//	calcOrigin(46, 100, &rayOrigin); 
//
//	//Calculate ray direction vector from pixel
//	vectorSubtraction(&rayOrigin, &projPoint, &rayDirection); 
//	normalizeVector(&rayDirection, &rayDirection);	
//
//	// calculate ray intersection distance
//	sphereIntersect(&rayOrigin, &rayDirection, &sphereCenter, &sphereRadius, &tValue);
//
//	loadFAC1fromMem(&tValue);
//	//printFAC1();
//	//cprintf("\n \r");
//	//FAC1toInt(&tValueInt);
//	//cprintf("%i \n \r", tValueInt);
//	FAC1toInt(&tValueInt);
//	//cprintf("%i \n \r", tValueInt);
//
//	if (tValueInt >= 0) {
//		cprintf("YES \n \r");
//	}
//	else {
//		cprintf("NO \n \r");
//	}
//
//
//
//	for (;;); // loop forever, never ends
//
//}

void setupRaytrace() {
	unsigned int i = 0;	
	unsigned char emptyPixel = 0b00000000;
	unsigned int memAddr;

	//bordercolor(1);
	bgcolor(0x06);
	
	POKE(0x0001, 0b00110111);		//ram Configuration
	POKE(0xdd00, 0b00000010);		//VIC configuration (CIA-2) - VIC Adresses $4000 - $7FFF
	POKE(0xd011, 0b00110100);		//set to bitmap mode
	POKE(0xd016, 0b00011000);		//set to multicolor mode
	POKE(0xd018, 0b00001000);		//set screenmem to slot 0 (+ 0) and bitmap pointer to slot 2 (+ $2000)

	
	memAddr = bitmapAdress;
	// Erase bitmap memory (blank screen)
	for (i = 0; i < 0x2000; i++) {
		POKE(memAddr + i, emptyPixel);
	}

	// set color matrix (color 1 and 2)
	memAddr = vicBankAdress;
	for (i = 0; i < 0x0400; i++) {
		POKE(memAddr + i, MYCOLORS);
	}

	// set color ram (color 3)
	memAddr = 0xD800; // Hardware Color RAM location
	for (i = 0; i < 0x0400; i++) {
		POKE(memAddr + i, MYCOLORS2);
	}
}

void main(void)
{
	struct vector3 rayOrigin;
	struct vector3 projPoint;
	struct vector3 rayDirection;
	struct vector3 sphereCenter;
	struct floatingPoint sphereRadius;
	struct floatingPoint tValue;
	unsigned int x = 0;						// pixel position x coordinate
	unsigned int y = 0;						// pixel position y coordinate
	unsigned char drawColor = 3;
	signed int tValueInt;

	//DEBUG CALC
	//FloatMathDebug();
	
	//SETUP
	setupRaytrace();

	//Make Float - 0.5
	makeFraction(1, 2, &half);

	//set projection angle point (change Z position to change lens angle)
	fillVectorValues(&projPoint, 0, 0, -1);

	//set Sphere center and radius
	fillVectorValues(&sphereCenter, 0, 0, 5);
	makeFPImmediate(2, &sphereRadius);


	// RENDER LOOP	
	for (y = 0; y < 200; y++) {
		for (x = 0; x < 160; x++) {

			//Calculate origin vector at pixel
			calcOrigin(x, y, &rayOrigin);

			//Calculate ray direction vector from pixel
			vectorSubtraction(&rayOrigin, &projPoint, &rayDirection);
			normalizeVector(&rayDirection, &rayDirection);

			// calculate ray intersection distance
			sphereIntersect(&rayOrigin, &rayDirection, &sphereCenter, &sphereRadius, &tValue);
			

			loadFAC1fromMem(&tValue);
			FAC1toInt(&tValueInt);
			//HER ER DET AVRUNDINGSFEIL VED HELTALL (3 -> 2)
			
			if (tValueInt >= 0) {
				drawPixelMBM(x, y, 3);
			}
			else {
				drawPixelMBM(x, y, 1);
			}

			
			//tValue holds distance from origin to intersection point
			//TODO:
			//Velg mellom floating point debug og render
			//Test rendering with single color (need pixel plot)
			//Add intersection point (origin + rayDirection * tValue)
			//Add normal calculation (sphere center - intersection point)
			//Add lambertian shading
			//Choose color according to labertian value (0.00-0.25, 0.25-0.50, 0.50-0.75, 0.75-1.00)
		}
	}
	for (;;); // loop forever, never ends
}