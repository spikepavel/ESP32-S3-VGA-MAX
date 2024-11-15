#include <Arduino.h>
#include <esp_lcd_panel_rgb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>



class Font
{
  public:
    const int firstChar;
    const int charCount;
    const unsigned char *pixels;
    const int charWidth;
    const int charHeight;
    Font(int charWidth, int charHeight, const unsigned char *pixels, int firstChar = 32, int charCount = 96)
        :firstChar(firstChar),
        charCount(charCount),
        pixels(pixels),
        charWidth(charWidth),
        charHeight(charHeight)
    {
    }
    
    bool valid(char ch) const
    {
        return ch >= firstChar && ch < firstChar + charCount;
    }
};


class VGA {
public:
  VGA() {}
  ~VGA() {}

  bool init(int width, int height);
  bool init();

  void vsyncWait();

int cursorX=0;
int cursorY=0;
int cursorBaseX=0;
uint16_t frontColor=65535;
uint16_t backColor=0;
Font *font;
bool autoScroll=true;

//inline graphics & print functions
void dot(int x, int y, uint16_t color);
void dotFast(int x, int y, uint16_t color);
void clear();
void xLine(int x0, int x1, int y, uint16_t color);
void vLine(int x,  int y,  int h, uint16_t color);
void line(int x1, int y1, int x2, int y2, uint16_t color);
void fillRect(int x, int y, int w, int h, uint16_t color);
void rect(int x, int y, int w, int h, int color);
void circle(int x, int y, int r, int color);
void fillCircle(int x, int y, int r, int color);
void ellipse(int x, int y, int rx, int ry, int color);
void fillEllipse(int x, int y, int rx, int ry, int color);
void drawChar(int x, int y, int ch);
void setFont(Font &font);
void setCursor(int x, int y);
void setTextColor(int front, int back = 0);
void print(const char ch);
void println(const char ch);
void print(const char *str);
void println(const char *str);
void print(long number, int base = 10, int minCharacters = 1);
void print(unsigned long number, int base = 10, int minCharacters = 1);
void println(long number, int base = 10, int minCharacters = 1);
void println(unsigned long number, int base = 10, int minCharacters = 1);
void print(int number, int base = 10, int minCharacters = 1);
void println(int number, int base = 10, int minCharacters = 1);
void print(unsigned int number, int base = 10, int minCharacters = 1);
void println(unsigned int number, int base = 10, int minCharacters = 1);
void print(short number, int base = 10, int minCharacters = 1);
void println(short number, int base = 10, int minCharacters = 1);
void print(unsigned short number, int base = 10, int minCharacters = 1);
void println(unsigned short number, int base = 10, int minCharacters = 1);
void print(unsigned char number, int base = 10, int minCharacters = 1);
void println(unsigned char number, int base = 10, int minCharacters = 1);
void print(double number, int fractionalDigits = 2, int minCharacters = 1);
void println(double number, int fractionalDigits = 2, int minCharacters = 1);
void println();
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
void cpLine(int x,int y ,int len);

	void mouse(int x1, int y1);
	void tri(int x1, int y1, int x2, int y2, int x3, int y3, int rgb);
	void fillTri(int x1, int y1, int x2, int y2, int x3, int y3, int rgb);
	void squircle(int center_x, int center_y, int a, int b, float n, int rgb);
	void fillSquircle(int center_x, int center_y, int a, int b, float n, int rgb);
	
	void scrollText(int dy);
	void drawRGBBitmap(int x,int y,uint16_t* bitmap,int w,int h);
	void drawCharScale(int x, int y, int ch, int scaleFont);
	void get(int x, int y);
	
	uint16_t getFn(int x, int y);
	
	uint16_t getBackFn(int x, int y);
	
	void dotAdd(int x, int y, int rgb1, int alpha);
	
	void FloodFill(int x, int y,int fill_color, int border_color);
	
	uint16_t* getDrawBuffer(); // получить основной буфер
	
	uint16_t* getBackBuffer(); // получить теневой буфер
		
protected:
  SemaphoreHandle_t _sem_vsync_end;
  SemaphoreHandle_t _sem_gui_ready;
  esp_lcd_panel_handle_t _panel_handle = NULL;
  int  _pixClock;
  uint16_t *_frBuf[2];
  int _fbIndex = 0;
  int _Width = 0;
  int _Height = 0;
  int _bounceLinesBuf = 0;
  int _lastBBuf = 0;
  //uint16_t* getDrawBuffer(); убрал из протектед
  void swapBuffers();
  static bool vsyncEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
  static bool bounceEvent(esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx);
};
