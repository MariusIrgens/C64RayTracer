#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <peekpoke.h>

#define vicBankAdress 0x4000
#define bitmapAdress 0x6000
#define RESOLUTION_X 160
#define RESOLUTION_Y 200
#define ORIGIN_Z 0
#define MYCOLORS 0xFC	// color 2, color 1
#define MYCOLORS2 0x01	// unused, color 3
#define NUMBER_OF_STEPS 80
#define MAX_TRACE_DISTANCE 5
#define MINIMUM_HIT_DISTANCE_FACTOR 10


// STRUCTS

// Floating points (needs 5 bytes in RAM)
struct floatingPoint {
	char exponent;
	char mantissa1;
	char mantissa2;
	char mantissa3;
	char mantissa4;
};

// vector3 of floating points (needs 3*5 = 15 bytes in RAM)
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
struct vector3 tempVector;
struct floatingPoint tempFPx;
struct floatingPoint tempFPy;
struct floatingPoint tempFPz;
struct floatingPoint half;
struct floatingPoint aspect;
struct floatingPoint MINIMUM_HIT_DISTANCE;
div_t xDiv;
div_t yDiv;


// FLOATING POINT MATH - using C64 Basic and Kernal ROM routines called with assembly
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
	__asm__("jsr $b850"); //subtract - FAC1 from value at mem adress (mem-FAC1)
}

void multiplyFACs() {
	__asm__("lda $61"); //to set zero flag
	__asm__("jsr $ba2b"); //multiplication - FAC1 and FAC2
}

void multiplyFAC1Mem(unsigned int adress) {
	LoadRegAYwithAdress(adress);
	__asm__("jsr $ba28"); //multiplication - FAC1 and value at mem adress
}

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

void vectorCopy(unsigned int sourceAdress, unsigned int destAdress) {
	//Load source.x and store it in dest.x 
	loadFAC1fromMem(sourceAdress);
	storeFAC1InMem(destAdress);

	//Load source.y and store it in dest.y
	loadFAC1fromMem(sourceAdress + 5);
	storeFAC1InMem(destAdress + 5);

	//Load source.z and store it in dest.z 
	loadFAC1fromMem(sourceAdress + 10);
	storeFAC1InMem(destAdress + 10);
}

void vectorAddition(unsigned int vecAdressA, unsigned int vecAdressB, unsigned int destAdress) {
	//Add B.x to A.x and store it in dest.x 
	loadFAC1fromMem(vecAdressA);
	addFAC1Mem(vecAdressB);
	storeFAC1InMem(destAdress);

	//Add B.y to A.y and store it in dest.y 
	loadFAC1fromMem(vecAdressA + 5);
	addFAC1Mem(vecAdressB + 5);
	storeFAC1InMem(destAdress + 5);

	//Add B.z to A.z and store it in dest.z 
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

void vectorMultiplyByMem(unsigned int vecAdress, unsigned int FPadress, unsigned int destAdress) {
	//Multiply A.x by FP and store it in dest.x 
	loadFAC1fromMem(vecAdress);
	multiplyFAC1Mem(FPadress);
	storeFAC1InMem(destAdress);

	//Multiply A.y by FP and store it in dest.y
	loadFAC1fromMem(vecAdress + 5);
	multiplyFAC1Mem(FPadress);
	storeFAC1InMem(destAdress + 5);

	//Multiply A.z by FP and store it in dest.z 
	loadFAC1fromMem(vecAdress + 10);
	multiplyFAC1Mem(FPadress);
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
	multiplyFAC1Mem(&aspect);			// Make up for resolution difference
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

//Calculate Sphere-Ray intersection point distance (nearest, if any).
//Based on the algorithm in the book "Mathematics for 3D game programming and computer graphics (3rd edition)"
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

void calcJumpPoint(unsigned int ro, unsigned int rd, unsigned int t, unsigned int destAdress) {
	// rayOrigin + rayDirection * t
	vectorMultiplyByMem(rd, t, destAdress); // rayDirection * t
	vectorAddition(ro, destAdress, destAdress); // add ro
}

//METHOD 2 FUNCTIONS
//Calculate Ray intersection point distance using the Signed Distance Function method, as popularized by Inigo Quilez
//https://iquilezles.org

void distanceFromSphere(unsigned int point, unsigned int sphereCenter, unsigned int sphereRadius, unsigned int destAdress) {
	// length(point - sphere_center) - sphere_radius;
	
	//(point - sphere_center)
	vectorSubtraction(point, sphereCenter, &tempVector);

	// length(point - sphere_center)
	vectorLength(&tempVector, &tempFPx);

	// (length(point - sphere_center)) - sphere_radius;

	// FAC2	- FAC1
	loadFAC2fromMem(&tempFPx);			//(length(point - sphere_center))
	loadFAC1fromMem(sphereRadius);		//sphere_radius
	subtractFACs();
	storeFAC1InMem(destAdress);

}

void mapTheWorld(unsigned int point, unsigned int destAdress) {
	struct vector3 sphereCenter;
	struct floatingPoint sphereRadius;
	struct floatingPoint distanceToClosest;
	fillVectorValues(&sphereCenter, 0, 0, 3);
	makeFPImmediate(2, &sphereRadius);

	//distance_to_closest = distance_from_sphere(point, sphere.center, sphere.radius));
	distanceFromSphere(point, &sphereCenter, &sphereRadius, &distanceToClosest);

	loadFAC1fromMem(&distanceToClosest);		//return distanceToClosest
	storeFAC1InMem(destAdress);

}

void SDFRaymarch(unsigned int ro, unsigned int rd, unsigned int destAdress) {
	unsigned int i;
	struct floatingPoint totalDistanceTraveled;
	struct floatingPoint distanceToClosest;
	struct vector3 currentPosition;
	signed int distanceToClosestInt;
	signed int totalDistanceTraveledInt;

	makeFPImmediate(0, &totalDistanceTraveled);									//totalDistanceTraveled = 0;
	vectorCopy(ro, &currentPosition);											//currentPosition = ray origin;
	makeFPImmediate(MAX_TRACE_DISTANCE, &distanceToClosest);					//distanceToClosest = MAX_TRACE_DISTANCE;

	for (i = 0; i < NUMBER_OF_STEPS; i++) {
		calcJumpPoint(ro, rd, &totalDistanceTraveled, &currentPosition);		//current_position = ray_origin + total_distance_traveled * ray_direction;
		mapTheWorld(&currentPosition, &distanceToClosest);						//distance_to_closest = map_the_world(current_position);
		
		loadFAC1fromMem(&totalDistanceTraveled);								//total_distance_traveled += distance_to_closest;
		addFAC1Mem(&distanceToClosest);											//
		storeFAC1InMem(&totalDistanceTraveled);									//

		loadFAC1fromMem(&distanceToClosest);									//Convert floatingpoint to int for comparison
		multiplyFAC1by10();														//Check below 0.1 (multiply more for different minimum distances)
		FAC1toInt(&distanceToClosestInt);										//

		if (distanceToClosestInt < 1) {											//Check if minimum distance has been reached
			loadFAC1fromMem(&totalDistanceTraveled);
			storeFAC1InMem(destAdress);											//return totalDistanceTraveled
			return;
		}

		loadFAC1fromMem(&totalDistanceTraveled);
		FAC1toInt(&totalDistanceTraveledInt);
		if (totalDistanceTraveledInt > MAX_TRACE_DISTANCE) {
			makeFPImmediate(-1, destAdress);
			return;
		}
	}
	makeFPImmediate(-1, destAdress);
}

//Draw pixel while using multicolor bitmap mode
void drawPixelMBM(unsigned int x, unsigned int y, unsigned char color) {
	unsigned char addedPixel;
	unsigned char existingPixel;
	unsigned int memAddr;
	xDiv = div(x, 4);
	yDiv = div(y, 8);

	if (color == 0) {
		return;
	}

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

	bordercolor(11);
	bgcolor(0);
	
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
	struct vector3 intersectionPoint;
	struct vector3 sphereCenter;
	struct vector3 lightPosition;
	struct vector3 pointToLightVec;
	struct vector3 pointNormalVec;
	struct floatingPoint sphereRadius;
	struct floatingPoint tValue;
	struct floatingPoint lambertian;
	unsigned int x = 0;						// pixel position x coordinate
	unsigned int y = 0;						// pixel position y coordinate
	unsigned int xStartEnd = 0;				// narrow the render window
	unsigned int yStartEnd = 0;				// narrow the render window
	unsigned char drawColor = 3;
	signed int tValueInt;
	signed int lambertianInt;

	//DEBUG CALC
	//FloatMathDebug();
	
	//SETUP
	setupRaytrace();

	//Make fraction floats
	makeFraction(1, 2, &half); //0.5
	makeFraction(RESOLUTION_X*2, RESOLUTION_Y, &aspect); //screen aspect ratio
	makeFraction(1, MINIMUM_HIT_DISTANCE_FACTOR, &MINIMUM_HIT_DISTANCE); //0.1


	//set projection angle point (change Z position to change lens angle)
	fillVectorValues(&projPoint, 0, 0, -1);

	//set Sphere center and radius (METHOD 1)
	//fillVectorValues(&sphereCenter, 0, 0, 3);
	//makeFPImmediate(2, &sphereRadius);

	//set light position
	fillVectorValues(&lightPosition, 1, 3, -2);


	//only render part of screen
	xStartEnd = 50; 
	yStartEnd = 50;

	// RENDER LOOP	
	for (y = yStartEnd; y < (RESOLUTION_Y - yStartEnd); y++) {
		for (x = xStartEnd; x < (RESOLUTION_X - xStartEnd); x++) {

			//Calculate origin vector at pixel
			calcOrigin(x, y, &rayOrigin);

			//Calculate ray direction vector from pixel
			vectorSubtraction(&rayOrigin, &projPoint, &rayDirection);
			normalizeVector(&rayDirection, &rayDirection);

			
			//METHOD 1: RAY-SPHERE INTERSECTION DISTANCE
			//sphereIntersect(&rayOrigin, &rayDirection, &sphereCenter, &sphereRadius, &tValue);

			//METHOD 2: SDF LOOP
			SDFRaymarch(&rayOrigin, &rayDirection, &tValue);


			// check if we actually hit anything before calculating intersection point 
			loadFAC1fromMem(&tValue);
			FAC1toInt(&tValueInt);
						
			// HIT
			if (tValueInt >= 0) {
				//get intersection point using distance t
				calcJumpPoint(&rayOrigin, &rayDirection, &tValue, &intersectionPoint);
				//calculate normalized point-to-light vector
				vectorSubtraction(&lightPosition, &intersectionPoint, &pointToLightVec);
				normalizeVector(&pointToLightVec, &pointToLightVec);
				//calculate point normal (METHOD 1)
				//vectorSubtraction(&intersectionPoint, &sphereCenter, &pointNormalVec);
				//normalizeVector(&pointNormalVec, &pointNormalVec);
				//calculate point normal (METHOD 2)
				fillVectorValues(&pointNormalVec, 0, 0, 3);
				
				//get lambertian (0-1) and multiply by 100 to get 0-100 integer
				dotProduct(&pointToLightVec, &pointNormalVec, &lambertian);
				loadFAC1fromMem(&lambertian);
				multiplyFAC1by10();
				multiplyFAC1by10();
				FAC1toInt(&lambertianInt);
				//TEST
				lambertianInt = 99;
				
				//choose render color defined by lambertian value
				//COLOR 1
				if (lambertianInt < 17) {
					//DITHER (0 and 1)
					yDiv = div(y, 2);
					xDiv = div(x+yDiv.rem, 2);
					if (xDiv.rem == 0) { drawColor = 0;	}
					else { drawColor = 1; }
				}
				//COLOR 2
				else if (lambertianInt < 35) {
					drawColor = 1;
				}
				//COLOR 3
				else if (lambertianInt < 53) {
					//DITHER (1 and 2)
					yDiv = div(y, 2);
					xDiv = div(x + yDiv.rem, 2);
					if (xDiv.rem == 0) { drawColor = 1; }
					else { drawColor = 2; }
				}
				//COLOR 4
				else if (lambertianInt < 71) {
					drawColor = 2;
				}
				//COLOR 5
				else if (lambertianInt < 89) {
					//DITHER (2 and 3)
					yDiv = div(y, 2);
					xDiv = div(x + yDiv.rem, 2);
					if (xDiv.rem == 0) { drawColor = 2; }
					else { drawColor = 3; }
				}
				//COLOR 6
				else {
					drawColor = 3;
				}
				drawPixelMBM(x, y, drawColor);
			}
			// MISS
			else {
				drawPixelMBM(x, y, 1);
			}
		}
	}
	for (;;); // loop forever, never ends
}