//пример добавлен исключительно в ознакомительных целях
//the example was added for informational purposes only

#include "VGA.h"
#include <FONT_9x16.h>

VGA vga;

int ScreenID = 0;

void setup()
{
  vga.init();
  vga.setFont(FONT_9x16);
  vga.setTextColor(65535,0);

	for(int y = 0; y < 600; y++)
		for(int x = 0; x < 800; x++)
			vga.dot(x, y, vga.rgb(x, y, 255-x));

	delay(5000);
}

void loop() 
{
vga.clear();
delay(1000);
for(int count = 0; count < 100000; count++)
vga.dot(rand()%800, rand()%600, rand()%65535);
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.line(rand()%800, rand()%600, rand()%800, rand()%600, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.tri(rand()%800, rand()%600, rand()%800, rand()%600, rand()%800, rand()%600, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 100; count++){
vga.fillTri(rand()%800, rand()%600, rand()%800, rand()%600, rand()%800, rand()%600, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.rect(rand()%800, rand()%600, rand()%800, rand()%600, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 100; count++){
vga.fillRect(rand()%800, rand()%600, rand()%800, rand()%600, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.circle(rand()%800, rand()%600, rand()%100, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 500; count++){
vga.fillCircle(rand()%800, rand()%600, rand()%50, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.ellipse(rand()%800, rand()%600, rand()%100, rand()%100, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 100; count++){
vga.fillEllipse(rand()%800, rand()%600, rand()%100, rand()%100, rand()%65535);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

vga.clear();
delay(1000);
for(int count = 0; count < 1000; count++){
vga.mouse(rand()%800, rand()%600);
delay(1);}
delay(1000);
vga.clear();
delay(1000);

for(int count = 0; count < 1000; count++)
{
	static int c = 0;
	static int d = 1;
	c += d;
	if (c == 0 || c == 255)
		d = -d;

	char text[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0
				  };
	
	for (int i = 0; i < 256; i++)
		text[i] = 33 + (i + (c >> 2));
	vga.setCursor(8, 48);
	vga.setTextColor(vga.rgb(c, 255 - c, 255), vga.rgb(0, c / 2, 127 - c / 2));
	vga.print(text);
	vga.setCursor(8, 148);
	vga.print(text);
	vga.setCursor(8, 248);
	vga.print(text);
	vga.setCursor(8, 348);
	vga.print(text);
}
 delay(4000);
}