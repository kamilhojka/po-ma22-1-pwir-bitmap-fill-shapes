// pwir-bitmap-fill-shapes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <thread>
#include <stack>
#include <omp.h>

using namespace std;

struct Point {
	int x;
	int y;
	Point() {};
	Point(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
};

struct Color {
	int r;
	int g;
	int b;
	Color() {};
	Color(int r, int g, int b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
	}
	bool Equals(Color color) {
		return color.b == this->b && color.g == this->g && color.r == this->r;
	}
};

struct BMP {
	unsigned char infoHeader[54];
	int width;
	int height;
	int paddingRow;
	unsigned char** data;

	BMP(const char* fname) {
		FILE* fpoint;
		errno_t err = fopen_s(&fpoint, fname, "rb");
		if (err != 0) {
			throw "Plik o tej nazwie nie istnieje";
		}
		fread(infoHeader, sizeof(unsigned char), 54, fpoint);
		width = *(int*)&infoHeader[18];
		height = *(int*)&infoHeader[22];
		paddingRow = (width * 3 + 3) & (~3);
		data = new unsigned char* [height];
		for (int i = 0; i < height; i++)
		{
			data[i] = new unsigned char[paddingRow];
			fread(data[i], sizeof(unsigned char), paddingRow, fpoint);
		}
		fclose(fpoint);
	}

	~BMP() {
		for (int i = 0; i < height; i++) {
			delete[] data[i];
		}
		delete[] data;
	}

	void write(const char* fname) {
		FILE* fpoint;
		errno_t err = fopen_s(&fpoint, fname, "wb");
		if (err != 0) {
			throw "Plik o tej nazwie nie istnieje";
		}
		fwrite(infoHeader, sizeof(unsigned char), 54, fpoint);
		for (int i = 0; i < height; i++) {
			fwrite(data[i], sizeof(unsigned char), paddingRow, fpoint);
		}
		fclose(fpoint);
	}

	Color getColor(int x, int y) {
		return Color(data[y][x * 3], data[y][x * 3 + 1], data[y][x * 3 + 2]);
	}

	void setColor(Point point, Color color) {
		data[point.y][point.x * 3] = color.b;
		data[point.y][point.x * 3 + 1] = color.g;
		data[point.y][point.x * 3 + 2] = color.r;
	}
};

void ShowIntroInformation(HANDLE hConsole);
void RunBitmapFilling(HANDLE hConsole);
void RunBitmapFillingParralelOpenMP(HANDLE hConsole);
void RunBitmapFillingParralelThread(HANDLE hConsole);

int main()
{
	setlocale(LC_CTYPE, "Polish");
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	ShowIntroInformation(hConsole);
	RunBitmapFilling(hConsole);
	RunBitmapFillingParralelOpenMP(hConsole);
	RunBitmapFillingParralelThread(hConsole);
}

void ShowIntroInformation(HANDLE hConsole)
{
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n\n  PROGRAMOWANIE WSPÓ£BIE¯NE I ROZPROSZONE 21/22L\n  Bitmap [500x500] - Wype³nianie kszta³tów\n  Autor programu: ";
	SetConsoleTextAttribute(hConsole, 15);
	cout << "Kamil Hojka -- 97632\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 14);
	cout << " Wype³nianie kszta³tów: kwadrat, trójk¹t, okr¹g w Bitmapie o rozmiarze 500x500";
	SetConsoleTextAttribute(hConsole, 15);
}

void FillShape(BMP& bitmap, const Color& color) {
	int* mask = new int[bitmap.width * bitmap.height]();
	stack<Point> points;
	Color backgroundColor = bitmap.getColor(0, 0);

	points.push(Point(0, 0));
	while (!points.empty()) {
		auto p = points.top();
		points.pop();
		mask[bitmap.height * p.x + p.y] = 2;

		for (int y = max(p.y - 1, 0); y <= min(p.y + 1, bitmap.height - 1); y++) {
			for (int x = max(p.x - 1, 0); x <= min(p.x + 1, bitmap.width - 1); x++) {
				if (mask[bitmap.height * x + y] == 0 && backgroundColor.Equals(bitmap.getColor(x, y)))
				{
					points.push(Point(x, y));
					mask[bitmap.height * x + y] = 1;
				}
			}
		}
	}

	for (int y = 0; y < bitmap.height; y++) {
		for (int x = 0; x < bitmap.width; x++) {
			if (bitmap.getColor(x, y).Equals(backgroundColor) && mask[bitmap.height * x + y] == 0) {
				bitmap.setColor(Point(x, y), color);
			}
		}
	}
	delete[] mask;
}

void RunBitmapFilling(HANDLE hConsole)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Sekwencyjne wype³nianie kszta³tów - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);

	try
	{
		BMP bitmapSquare("square.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShape(bitmapSquare, Color(255, 0, 0));
		bitmapSquare.write("output_square_sequentially.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla kwadratu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (square.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapTriangle("triangle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShape(bitmapTriangle, Color(0, 255, 0));
		bitmapTriangle.write("output_triangle_sequentially.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla trójk¹tu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (triangle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapCircle("circle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShape(bitmapCircle, Color(0, 0, 255));
		bitmapCircle.write("output_circle_sequentially.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla okrêgu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (circle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}

void FillShapeParralelThread(BMP& bitmap, const Color& color) {
	int* mask = new int[bitmap.width * bitmap.height]();
	stack<Point> points;
	Color backgroundColor = bitmap.getColor(0, 0);

	points.push(Point(0, 0));
	while (!points.empty()) {
		auto p = points.top();
		points.pop();
		mask[bitmap.height * p.x + p.y] = 2;

		for (int y = max(p.y - 1, 0); y <= min(p.y + 1, bitmap.height - 1); y++) {
			for (int x = max(p.x - 1, 0); x <= min(p.x + 1, bitmap.width - 1); x++) {
				if (mask[bitmap.height * x + y] == 0 && backgroundColor.Equals(bitmap.getColor(x, y)))
				{
					points.push(Point(x, y));
					mask[bitmap.height * x + y] = 1;
				}
			}
		}
	}

	thread threads[500];
	auto threadFunction = [](BMP& bitmap, int* mask, const Color& backgroundColor, const Color& color, int thread) 
	{
		double slice = (bitmap.height * 1.0) / 500;
		for (int y = slice * thread; y < slice * (thread + 1); y++) {
			for (int x = 0; x < bitmap.width; x++) {
				if (bitmap.getColor(x, y).Equals(backgroundColor) && mask[bitmap.height * x + y] == 0) {
					bitmap.setColor(Point(x, y), color);
				}
			}
		}
	};

	for (int i = 0; i < 500; i++) {
		threads[i] = thread(threadFunction, ref(bitmap), mask, ref(backgroundColor), ref(color), i);
	}
	for (int i = 0; i < 500; i++) {
		threads[i].join();
	}
	delete[] mask;
}

void RunBitmapFillingParralelThread(HANDLE hConsole)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Równoleg³e wype³nianie kszta³tów za pomoc¹ thread - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);

	try
	{
		BMP bitmapSquare("square.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelThread(bitmapSquare, Color(255, 0, 0));
		bitmapSquare.write("output_square_parralel_thread.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla kwadratu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (square.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapTriangle("triangle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelThread(bitmapTriangle, Color(0, 255, 0));
		bitmapTriangle.write("output_triangle_parralel_thread.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla trójk¹tu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (triangle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapCircle("circle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelThread(bitmapCircle, Color(0, 0, 255));
		bitmapCircle.write("output_circle_parralel_thread.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla okrêgu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (circle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}

void FillShapeParralelOpenMP(BMP& bitmap, const Color& color) {
	int* mask = new int[bitmap.width * bitmap.height]();
	stack<Point> points;
	Color backgroundColor = bitmap.getColor(0, 0);

	points.push(Point(0, 0));
	while (!points.empty()) {
		auto p = points.top();
		points.pop();
		mask[bitmap.height * p.x + p.y] = 2;

		for (int y = max(p.y - 1, 0); y <= min(p.y + 1, bitmap.height - 1); y++) {
			for (int x = max(p.x - 1, 0); x <= min(p.x + 1, bitmap.width - 1); x++) {
				if (mask[bitmap.height * x + y] == 0 && backgroundColor.Equals(bitmap.getColor(x, y)))
				{
					points.push(Point(x, y));
					mask[bitmap.height * x + y] = 1;
				}
			}
		}
	}

#pragma omp parallel for
	for (int y = 0; y < bitmap.height; y++) {
		for (int x = 0; x < bitmap.width; x++) {
			if (bitmap.getColor(x, y).Equals(backgroundColor) && mask[bitmap.height * x + y] == 0) {
				bitmap.setColor(Point(x, y), color);
			}
		}
	}
	delete[] mask;
}

void RunBitmapFillingParralelOpenMP(HANDLE hConsole)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Równoleg³e wype³nianie kszta³tów za pomoc¹ OpenMP - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);

	try
	{
		BMP bitmapSquare("square.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelOpenMP(bitmapSquare, Color(255, 0, 0));
		bitmapSquare.write("output_square_parralel_openmp.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla kwadratu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (square.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapTriangle("triangle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelOpenMP(bitmapTriangle, Color(0, 255, 0));
		bitmapTriangle.write("output_triangle_parralel_openmp.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla trójk¹tu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (triangle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}

	try
	{
		BMP bitmapCircle("circle.bmp");
		auto begin = chrono::high_resolution_clock::now();
		FillShapeParralelOpenMP(bitmapCircle, Color(0, 0, 255));
		bitmapCircle.write("output_circle_parralel_openmp.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas dla okrêgu: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (circle.bmp)\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}