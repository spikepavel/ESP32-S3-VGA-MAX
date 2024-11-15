#include "VGA.h"
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_async_memcpy.h>

const async_memcpy_config_t async_mem_cfg = {
        .backlog = 1280,
        .sram_trans_align = 64,
        .psram_trans_align = 64,
        .flags = 0
    };
async_memcpy_t async_mem_handle;

#define VGA_PIN_NUM_HSYNC          1
#define VGA_PIN_NUM_VSYNC          2
#define VGA_PIN_NUM_DE             -1 //LCD only(optional)

#define VGA_PIN_NUM_PCLK           -1// any unused pin. For LCD needed real IO-pin

#define VGA_PIN_NUM_DATA0          4  //R0
#define VGA_PIN_NUM_DATA1          5  //R1
#define VGA_PIN_NUM_DATA2          6  //R2
#define VGA_PIN_NUM_DATA3          7  //R3
#define VGA_PIN_NUM_DATA4          8  //R4

#define VGA_PIN_NUM_DATA5          9   //G0
#define VGA_PIN_NUM_DATA6          10  //G1
#define VGA_PIN_NUM_DATA7          11  //G2
#define VGA_PIN_NUM_DATA8          12  //G3
#define VGA_PIN_NUM_DATA9          13  //G4
#define VGA_PIN_NUM_DATA10         14  //G5

#define VGA_PIN_NUM_DATA11         15  //B0
#define VGA_PIN_NUM_DATA12         16  //B1
#define VGA_PIN_NUM_DATA13         17  //B2
#define VGA_PIN_NUM_DATA14         18  //B3
#define VGA_PIN_NUM_DATA15         21  //B4

#define VGA_PIN_NUM_DISP_EN        -1 //LCD only
bool VGA::init(){
  return init(800,600);
}
bool VGA::init(int width, int height) {
  esp_async_memcpy_install(&async_mem_cfg,&async_mem_handle);
  // temp replace
  _Width = width;
  _Height = height;
  _bounceLinesBuf = height / 10;
  _pixClock = 32000000;
  _sem_vsync_end = xSemaphoreCreateBinary();
  assert(_sem_vsync_end);
  _sem_gui_ready = xSemaphoreCreateBinary();
  assert(_sem_gui_ready);
  esp_lcd_rgb_panel_config_t panel_config;
  memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));
  panel_config.data_width = 16;
  panel_config.bits_per_pixel= 16;
  panel_config.psram_trans_align = 64;
  panel_config.dma_burst_size = 64;
  panel_config.num_fbs = 0;
  panel_config.clk_src = LCD_CLK_SRC_PLL240M;
  panel_config.bounce_buffer_size_px = _bounceLinesBuf * _Width;
  panel_config.disp_gpio_num = VGA_PIN_NUM_DISP_EN;
  panel_config.pclk_gpio_num = VGA_PIN_NUM_PCLK;
  panel_config.vsync_gpio_num = VGA_PIN_NUM_VSYNC;
  panel_config.hsync_gpio_num = VGA_PIN_NUM_HSYNC;
  panel_config.de_gpio_num = VGA_PIN_NUM_DE;
  panel_config.data_gpio_nums[0] = VGA_PIN_NUM_DATA0;
  panel_config.data_gpio_nums[1] = VGA_PIN_NUM_DATA1;
  panel_config.data_gpio_nums[2] = VGA_PIN_NUM_DATA2;
  panel_config.data_gpio_nums[3] = VGA_PIN_NUM_DATA3;
  panel_config.data_gpio_nums[4] = VGA_PIN_NUM_DATA4;
  panel_config.data_gpio_nums[5] = VGA_PIN_NUM_DATA5;
  panel_config.data_gpio_nums[6] = VGA_PIN_NUM_DATA6;
  panel_config.data_gpio_nums[7] = VGA_PIN_NUM_DATA7;
  panel_config.data_gpio_nums[8] = VGA_PIN_NUM_DATA8;
  panel_config.data_gpio_nums[9] = VGA_PIN_NUM_DATA9;
  panel_config.data_gpio_nums[10] = VGA_PIN_NUM_DATA10;
  panel_config.data_gpio_nums[11] = VGA_PIN_NUM_DATA11;
  panel_config.data_gpio_nums[12] = VGA_PIN_NUM_DATA12;
  panel_config.data_gpio_nums[13] = VGA_PIN_NUM_DATA13;
  panel_config.data_gpio_nums[14] = VGA_PIN_NUM_DATA14;
  panel_config.data_gpio_nums[15] = VGA_PIN_NUM_DATA15;
  panel_config.timings.pclk_hz = _pixClock;
  panel_config.timings.h_res = _Width;
  panel_config.timings.v_res = _Height;
        panel_config.timings.hsync_back_porch = 88;
        panel_config.timings.hsync_front_porch = 40;
        panel_config.timings.hsync_pulse_width = 128;
        panel_config.timings.vsync_back_porch = 23;
        panel_config.timings.vsync_front_porch = 1;
        panel_config.timings.vsync_pulse_width = 4;
  panel_config.timings.flags.pclk_active_neg = false;
  panel_config.timings.flags.hsync_idle_low = 0;
  panel_config.timings.flags.vsync_idle_low = 0;
  panel_config.flags.fb_in_psram = 0;
  panel_config.flags.double_fb = 0;
  panel_config.flags.no_fb = 1;
  esp_lcd_new_rgb_panel(&panel_config, &_panel_handle);
  int fbSize = _Width*_Height*2; // *2 два байта на пиксель
  for (int i = 0; i < 2; i++) {
        _frBuf[i] = (uint16_t*)heap_caps_aligned_alloc(64,fbSize,MALLOC_CAP_SPIRAM);
        assert(_frBuf[i]);
  }
  memset(_frBuf[1], 0, fbSize);
  memset(_frBuf[0], 0, fbSize);
  _lastBBuf = _Width*(_Height-_bounceLinesBuf);
  esp_lcd_rgb_panel_event_callbacks_t cbs = {
      .on_vsync = vsyncEvent,
      .on_bounce_empty = bounceEvent,
  };
  esp_lcd_rgb_panel_register_event_callbacks(_panel_handle, &cbs, this);
  esp_lcd_panel_reset(_panel_handle);
  esp_lcd_panel_init(_panel_handle);
    return true;
}

extern int ScreenID; // указатель в какой буфер мы пишем, смотрим прилагаемый пример, или пишем мне за помощью

void VGA::vsyncWait() {
  // get draw semaphore
    xSemaphoreGive(_sem_gui_ready);
    xSemaphoreTake(_sem_vsync_end, 0);
}

uint16_t* VGA::getDrawBuffer() { // ручной ввод буфера экрана
if(ScreenID == 0)
    return _frBuf[0];

else if(ScreenID == 1)
    return _frBuf[1];

else if(ScreenID == 2)
{
  if (_fbIndex == 1) {
    return _frBuf[0];
  } else {
    return _frBuf[1];
  }		
}	
}

uint16_t* VGA::getBackBuffer() {
	return _frBuf[1];
}

bool VGA::vsyncEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {
    return true;
}

void VGA::swapBuffers() {
    if (xSemaphoreTakeFromISR(_sem_gui_ready, NULL) == pdTRUE) {
        if (_fbIndex == 0) {
            _fbIndex = 1;
        } else {
            _fbIndex = 0;
        }
        xSemaphoreGiveFromISR(_sem_vsync_end, NULL);
    }
}

bool VGA::bounceEvent(esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx) {
    VGA* vga = (VGA*)user_ctx;
    uint16_t* bbuf = (uint16_t*)bounce_buf;
    int screenLineIndex = pos_px / vga->_Width;
    int bufLineIndex = screenLineIndex;
    uint16_t* pptr = vga->_frBuf[vga->_fbIndex] + (bufLineIndex*vga->_Width);
    int screenLines = len_bytes / vga->_Width;
    int lines = screenLines;
    uint16_t* bbptr = (uint16_t*)bbuf;
    int copyBytes = lines*vga->_Width;
    esp_async_memcpy(async_mem_handle,bbptr,pptr,copyBytes,NULL,NULL);
    if (pos_px >= vga->_lastBBuf) {
        vga->swapBuffers();
    }
    return true;
}

void VGA::dot(int x, int y, uint16_t color)
{ 	
	if(x >= 0)
	{
	if(x <= 799)
	{
	if(y >= 0)
	{
	if(y <= 599)
	{
		
	//_frBuf[0][x + y * _Width] = color; другая запись, по скорости также
			
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    ptr+=x;
    ptr+=y*_Width;
    *ptr=color;
	
	}
	}
	}
	}
}

void VGA::dotFast(int x, int y, uint16_t color)
{ 			
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    ptr+=x;
    ptr+=y*_Width;
    *ptr=color;
}

void VGA::clear()
{
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    memset(ptr, 0, 960000);
}


void VGA::xLine(int x0, int x1, int y, uint16_t color)
  {

    if (y < 0 || y >= _Height) return;
    if (x0 > x1)
    {
      int xb = x0;
      x0 = x1;
      x1 = xb;
    }
    if (x0 < 0)
      x0 = 0;
    if (x1 > _Width)
      x1 = _Width;
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    ptr+=x0;
    ptr+=y*_Width;
    for (int i = x0; i < x1; i++){*ptr=color;ptr++;}
  
}

void VGA::vLine(int x,int y, int h, uint16_t color){
  uint16_t *ptr = (uint16_t *)getDrawBuffer();
  ptr+=x;
  ptr+=y*_Width;
  for(int i = 0; i < h;i++){
    *ptr=color;
    ptr+=_Width;
  }
}

void VGA::line(int x1, int y1, int x2, int y2, uint16_t color)
  {
    int x, y, xe, ye;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int dx1 = labs(dx);
    int dy1 = labs(dy);
    int px = 2 * dy1 - dx1;
    int py = 2 * dx1 - dy1;
    if (dy1 <= dx1)
    {
      if (dx >= 0)
      {
        x = x1;
        y = y1;
        xe = x2;
      }
      else
      {
        x = x2;
        y = y2;
        xe = x1;
      }
      dot(x, y, color);
      for (int i = 0; x < xe; i++)
      {
        x = x + 1;
        if (px < 0)
        {
          px = px + 2 * dy1;
        }
        else
        {
          if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
          {
            y = y + 1;
          }
          else
          {
            y = y - 1;
          }
          px = px + 2 * (dy1 - dx1);
        }
        dot(x, y, color);
      }
    }
    else
    {
      if (dy >= 0)
      {
        x = x1;
        y = y1;
        ye = y2;
      }
      else
      {
        x = x2;
        y = y2;
        ye = y1;
      }

      dot(x, y, color);
      for (int i = 0; y < ye; i++)
      {
        y = y + 1;
        if (py <= 0)
        {
          py = py + 2 * dx1;
        }
        else
        {
          if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
          {
            x = x + 1;
          }
          else
          {
            x = x - 1;
          }
          py = py + 2 * (dx1 - dy1);
        }
        dot(x, y, color);
      }
    }
  }

void VGA::fillRect(int x, int y, int w, int h, uint16_t color)
  {
    if (x < 0)
    {
      w += x;
      x = 0;
    }
    if (y < 0)
    {
      h += y;
      y = 0;
    }
    if (x + w > _Width)
      w = _Width - x;
    if (y + h > _Height)
      h = _Height - y;
      int i=0;
    for (int j = y; j < y + h; j++)
    {
      xLine(x,x+w,y+i,color);
      i++;
      }
  }

void VGA::rect(int x, int y, int w, int h, int color)
  {
    xLine(x,x+w,y,color);
    vLine(x, y, h, color);
    xLine(x,x+w,y+h-1,color);
    vLine(x + w - 1, y, h, color);
  }

void VGA::circle(int x, int y, int r, int color)
  {
    int oxr = r;
    for(int i = 0; i < r + 1; i++)
    {
      int xr = (int)sqrtf(r * r - i * i);
      xLine(x - oxr, x - xr + 1, y + i, color);
      xLine(x + xr, x + oxr + 1, y + i, color);
      if(i) 
      {
        xLine(x - oxr, x - xr + 1, y - i, color);
        xLine(x + xr, x + oxr + 1, y - i, color);
      }
      oxr = xr;
    }
  }

void VGA::fillCircle(int x, int y, int r, int color)
  {
    for(int i = 0; i < r + 1; i++)
    {
      int xr = (int)sqrtf(r * r - i * i);
      xLine(x - xr, x + xr + 1, y + i, color);
      if(i) 
        xLine(x - xr, x + xr + 1, y - i, color);
    }
  }

void VGA::ellipse(int x, int y, int rx, int ry, int color)
  {
    if(ry == 0)
      return;
    int oxr = rx;
    float f = float(rx) / ry;
    f *= f;
    for(int i = 0; i < ry + 1; i++)
    {
      float s = rx * rx - i * i * f;
      int xr = (int)sqrtf(s <= 0 ? 0 : s);
      xLine(x - oxr, x - xr + 1, y + i, color);
      xLine(x + xr, x + oxr + 1, y + i, color);
      if(i) 
      {
        xLine(x - oxr, x - xr + 1, y - i, color);
        xLine(x + xr, x + oxr + 1, y - i, color);
      }
      oxr = xr;
    }
  }

void VGA::fillEllipse(int x, int y, int rx, int ry, int color)
  {
    if(ry == 0)
      return;
    float f = float(rx) / ry;
    f *= f;   
    for(int i = 0; i < ry + 1; i++)
    {
      float s = rx * rx - i * i * f;
      int xr = (int)sqrtf(s <= 0 ? 0 : s);
      xLine(x - xr, x + xr + 1, y + i, color);
      if(i) 
        xLine(x - xr, x + xr + 1, y - i, color);
    }
  }


void VGA::drawChar(int x, int y, int ch)
    {
        if (!font)
            return;
        if (!font->valid(ch))
            return;
        const unsigned char *pix = &font->pixels[font->charWidth * font->charHeight * (ch - font->firstChar)];

        for (int py = 0; py < font->charHeight; py++)
            for (int px = 0; px < font->charWidth; px++)
                if (*(pix++))
                    dot(px + x, py + y, frontColor);
                else
                    dot(px + x, py + y, backColor);
}

void VGA::drawCharScale(int x, int y, int ch, int scaleFont)  // позволяет выводить по заданным координатам символы от 0 до 255 с масштабированием. Сделал по просьбе ребят с Волгограда. У них на Векторе-06Ц такая функция встроена аппаратно в железо. Для совместимости программ.
{
		
	if (!font)
		return;
	if (!font->valid(ch))
		return;
	const unsigned char *pix = &font->pixels[font->charWidth * font->charHeight * (ch - font->firstChar)];

	for(int yy = 0; yy < font->charHeight; yy++) {
	for(int xx = 0; xx < font->charWidth; xx++) {
		int	xScale = xx*scaleFont;
		int	yScale = yy*scaleFont;
    for(int ycount = 0; ycount < scaleFont; ycount++) {
    for(int xcount = 0; xcount < scaleFont; xcount++) {  
		if (*(pix))
		{
		dot(x + xScale + xcount, y + yScale + ycount, frontColor);			
		}
		}
		}	
		pix++;
		}
		}				
}

void VGA::setFont(Font &font)
    {
        this->font = &font;
    }

void VGA::setCursor(int x, int y)
    {   
        cursorX = cursorBaseX = x;
        cursorY = y;
    }
    
   
void VGA::setTextColor(int front, int back)
    {
        frontColor = front;
        backColor = back;
    }

void VGA::print(const char ch)
{
	if (!font)
		return;
	if (font->valid(ch))
	{
		drawChar(cursorX, cursorY, ch);
	}
	else
	{
		drawChar(cursorX, cursorY, ' ');
	}			
		cursorX += font->charWidth;
		
	if (cursorX + font->charWidth > 800)
	{
		cursorX = cursorBaseX;
		cursorY += font->charHeight;
		if(autoScroll && cursorY + font->charHeight > 599)
		{				
			scrollText(cursorY + font->charHeight - 599);
		}	
	}
}

void VGA::println(const char ch)
    {
        print(ch);
        print("\n");
    }

void VGA::print(const char *str)
    {
        if (!font)
            return;
        while (*str)
        {
            if(*str == '\n')
            {
                cursorX = cursorBaseX;
                cursorY += font->charHeight;
            }
            else
                print(*str);
            str++;
        }
    }

void VGA::println(const char *str)
    {   
        print(str);
        print("\n");
    }
void VGA::print(long number, int base, int minCharacters)
    {
        if(minCharacters < 1)
            minCharacters = 1;
        bool sign = number < 0;
        if (sign)
            number = -number;
        const char baseChars[] = "0123456789ABCDEF";
        char temp[33];
        temp[32] = 0;
        int i = 31;
        do
        {
            temp[i--] = baseChars[number % base];
            number /= base;
        } while (number > 0);
        if (sign)
            temp[i--] = '-';
        for (; i > 31 - minCharacters; i--)
            temp[i] = ' ';
        print(&temp[i + 1]);
    }

void VGA::print(unsigned long number, int base, int minCharacters)
  {
    if(minCharacters < 1)
      minCharacters = 1;
    const char baseChars[] = "0123456789ABCDEF";
    char temp[33];
    temp[32] = 0;
    int i = 31;
    do
    {
      temp[i--] = baseChars[number % base];
      number /= base;
    } while (number > 0);
    for (; i > 31 - minCharacters; i--)
      temp[i] = ' ';
    print(&temp[i + 1]);
  } 

void VGA::println(long number, int base, int minCharacters)
  {
    print(number, base, minCharacters); print("\n");
  }

void VGA::println(unsigned long number, int base, int minCharacters)
  {
    print(number, base, minCharacters); print("\n");
  }

void VGA::print(int number, int base, int minCharacters)
  {
    print(long(number), base, minCharacters);
  }

void VGA::println(int number, int base, int minCharacters)
  {
    println(long(number), base, minCharacters);
  }

void VGA::print(unsigned int number, int base, int minCharacters)
  {
    print((unsigned long)(number), base, minCharacters);
  }

void VGA::println(unsigned int number, int base, int minCharacters)
  {
    println((unsigned long)(number), base, minCharacters);
  }

void VGA::print(short number, int base, int minCharacters)
  {
    print(long(number), base, minCharacters);
  }

void VGA::println(short number, int base, int minCharacters)
  {
    println(long(number), base, minCharacters);
  }

void VGA::print(unsigned short number, int base, int minCharacters)
  {
    print(long(number), base, minCharacters);
  }

void VGA::println(unsigned short number, int base, int minCharacters)
  {
    println(long(number), base, minCharacters);
  }

void VGA::print(unsigned char number, int base, int minCharacters)
  {
    print(long(number), base, minCharacters);
  }

void VGA::println(unsigned char number, int base, int minCharacters)
  {
    println(long(number), base, minCharacters);
  }

void VGA::println()
  {
    print("\n");
  }

void VGA::print(double number, int fractionalDigits, int minCharacters)
  {
    long p = long(pow(10, fractionalDigits));
    long long n = (double(number) * p + 0.5f);
    print(long(n / p), 10, minCharacters - 1 - fractionalDigits);
    if(fractionalDigits)
    {
      print(".");
      for(int i = 0; i < fractionalDigits; i++)
      {
        p /= 10;
        print(long(n / p) % 10);
      }
    }
  }

void VGA::println(double number, int fractionalDigits, int minCharacters)
  {
    print(number, fractionalDigits, minCharacters); 
    print("\n");
  }

uint16_t VGA::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
}

void VGA::cpLine(int x,int y ,int len){
	
  uint16_t * line_ptr = (uint16_t *)getDrawBuffer();
  
  uint16_t * ptr = (uint16_t *)getDrawBuffer() - 1;
  
  ptr+=x;
  
  ptr+=y*_Width;
  
  memcpy(ptr,line_ptr,len*2);  
  
}

void VGA::FloodFill(int x, int y,int fill_color, int border_color)	//заливка ограниченной области до 400 пикселей, тестирую только
{		
  int x_left,x_right,YY,i;	
  int XMAX,YMAX;	
  XMAX=800;	
  YMAX=600;	
	
  x_left=x_right=x;	
  while (getFn(x_left,y)!=border_color && x_left>0)	
  {	
    dotFast(x_left,y,fill_color);	 
     x_left--;	
  }	
  x_right++;	
  while (getFn(x_right,y)!=border_color && x_right<XMAX)	
  {	
     dotFast(x_right,y,fill_color);		 
     x_right++;	
  }	


  x=(x_right+x_left)>>1;
  
  for(i=-1;i<=1;i+=2)	
  {	
     YY=y;	
     while (getFn(x,YY)!=border_color && YY<YMAX && YY>0) YY+=i;	
     YY=(y+YY)>>1;	
     if (getFn(x,YY)!=border_color && getFn(x,YY)!=fill_color) FloodFill(x,YY,fill_color,border_color);	
  }	 
}	

void VGA::scrollText(int dy) // тут можно сделать динамичный цвет по желанию
{
	uint16_t * line_ptr = (uint16_t *)getDrawBuffer();
    line_ptr+= font->charHeight*_Width;
  
	uint16_t * ptr = (uint16_t *)getDrawBuffer();
 
	memcpy(ptr, line_ptr, 960000 - 1600*font->charHeight);
	
	for (int i = 0; i <= font->charHeight; i++)
	{
	xLine(0, 800, 599 - i, 0);
	}				
	cursorY -= dy;
}

void VGA::mouse(int x1, int y1)   // рисует курсор мыши
{	
	const int mouse12_19[] =
	{
	0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  
	0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  
	0, 65535, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,  
	0, 65535, 65535, 0, -1, -1, -1, -1, -1, -1, -1, -1,  
	0, 65535, 65535, 65535, 0, -1, -1, -1, -1, -1, -1, -1, 
	0, 65535, 65535, 65535, 65535, 0, -1, -1, -1, -1, -1, -1,  
	0, 65535, 65535, 65535, 65535, 65535, 0, -1, -1, -1, -1, -1,  
	0, 65535, 65535, 65535, 65535, 65535, 65535, 0, -1, -1, -1, -1,  
	0, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 0, -1, -1, -1, 
	0, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 0, -1, -1,  
	0, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 0, -1, 
	0, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 0, 
	0, 65535, 65535, 65535, 65535, 65535, 65535, 0, 0, 0, 0, 0, 
	0, 65535, 65535, 65535, 0, 65535, 65535, 0, -1, -1, -1, -1,  
	0, 65535, 65535, 0, -1, 0, 65535, 65535, 0, -1, -1, -1, 
	0, 65535, 0, -1, -1, 0, 65535, 65535, 0, -1, -1, -1, 
	0, 0, -1, -1, -1, -1, 0, 65535, 65535, 0, -1, -1, 
	-1, -1, -1, -1, -1, -1, 0, 65535, 65535, 0, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1,	
	};
	
	int rrr ;	
    for(int y = 0; y < 19; y++) {
    for(int x = 0; x < 12; x++) {		
		int col = mouse12_19[rrr] ;	  	
	 if (col >=0)			// -1 мы не отображаем пиксель. 
		{
		dot(x + x1, y + y1, col);	  
		rrr = rrr + 1;	
		}		 
	 else
		{
		rrr = rrr + 1;
		}	  
		}					
	}		
	rrr = 0;
}


// графические примитивы
// graphic primitives

// это для заполненного треугольника подготовка *************

#define fixed int
#define round(x) floor(x + 0.5)
#define roundf(x) floor(x + 0.5f)


inline void SSswap(int &a, int &b)   //я сделал еще один SWAP (назвал SSswap)
{
      int t;
      t = a;
      a = b;
      b = t;
}

// представляем целое число в формате чисел с фиксированной точкой
inline fixed int_to_fixed(int value)
{
	return (value << 16);
}

// целая часть числа с фиксированной точкой
inline int fixed_to_int(fixed value)
{
	if (value < 0) return (value >> 16 - 1);
	if (value >= 0) return (value >> 16);
}

// округление до ближайшего целого
inline int round_fixed(fixed value)
{
	return fixed_to_int(value + (1 << 15));
}

// представляем число с плавающей точкой в формате чисел с фиксированной точкой
// здесь происходят большие потери точности
inline fixed double_to_fixed(double value)
{
	return round(value * (65536.0));
}

inline fixed float_to_fixed(float value)
{
	return roundf(value * (65536.0f));
}

// записываем отношение (a / b) в формате чисел с фиксированной точкой
inline fixed frac_to_fixed(int a, int b)
{
	return (a << 16) / b;
}

// окончание кода подготовки для заполненного треугольника *************


void VGA::tri(int x1, int y1, int x2, int y2, int x3, int y3, int rgb)    //рисуем треугольник (команда Бейсик TRI).
{		
	line(x1, y1, x2, y2, rgb);
	line(x2, y2, x3, y3, rgb);
	line(x3, y3, x1, y1, rgb);	
}

// ==============================начало отрисовки треугольника заполненного
	
void VGA::fillTri(int x1, int y1, int x2, int y2, int x3, int y3, int rgb) //Тут мы отрисовываем сам заполненный треугольник(команда Бейсик TRIC).
{
	// Упорядочиваем точки p1(x1, y1),
	// p2(x2, y2), p3(x3, y3)
  if (y2 < y1) {
    SSswap(y1, y2);
    SSswap(x1, x2);
  } // точки p1, p2 упорядочены
  if (y3 < y1) {
    SSswap(y1, y3);
    SSswap(x1, x3);
  } // точки p1, p3 упорядочены
	// теперь p1 самая верхняя
	// осталось упорядочить p2 и p3
  if (y2 > y3) {
    SSswap(y2, y3);
    SSswap(x2, x3);
  }

	// приращения по оси x для трёх сторон
	// треугольника
  fixed dx13 = 0, dx12 = 0, dx23 = 0;

	// вычисляем приращения
	// в случае, если ординаты двух точек
	// совпадают, приращения
	// полагаются равными нулю
  if (y3 != y1) {
    dx13 = int_to_fixed(x3 - x1);
    dx13 /= y3 - y1;
  }
  else
  {
    dx13 = 0;
  }

  if (y2 != y1) {
    dx12 = int_to_fixed(x2 - x1);
    dx12 /= (y2 - y1);
  }
  else
  {
    dx12 = 0;
  }

  if (y3 != y2) {
    dx23 = int_to_fixed(x3 - x2);
    dx23 /= (y3 - y2);
  }
  else
  {
    dx23 = 0;
  }

	// "рабочие точки"
	// изначально они находятся в верхней точке
  fixed wx1 = int_to_fixed(x1);
  fixed wx2 = wx1;

	// сохраняем приращение dx13 в другой переменной
  int _dx13 = dx13;

	// упорядочиваем приращения таким образом, чтобы
	// в процессе работы алгоритмы
	// точка wx1 была всегда левее wx2
  if (dx13 > dx12)
  {
    SSswap(dx13, dx12);
  }

	// растеризуем верхний полутреугольник
  for (int i = y1; i < y2; i++){
    // рисуем горизонтальную линию между рабочими точками
    for (int j = fixed_to_int(wx1); j <= fixed_to_int(wx2); j++){
	dot(j, i, rgb);
    }
    wx1 += dx13;
    wx2 += dx12;
  }

	// вырожденный случай, когда верхнего полутреугольника нет
	// надо разнести рабочие точки по оси x,
	// т.к. изначально они совпадают
  if (y1 == y2){
    wx1 = int_to_fixed(x1);
    wx2 = int_to_fixed(x2);
  }

	// упорядочиваем приращения
	// (используем сохраненное приращение)
  if (_dx13 < dx23)
  {
    SSswap(_dx13, dx23);
  }
  
	// растеризуем нижний полутреугольник
  for (int i = y2; i <= y3; i++){
    // рисуем горизонтальную линию между рабочими точками
    for (int j = fixed_to_int(wx1); j <= fixed_to_int(wx2); j++){
	dot(j, i, rgb);
    }
    wx1 += _dx13;
    wx2 += dx23;
  }
}

// ================================ конец отрисовки треугольника заполненного

void VGA::squircle(int center_x, int center_y, int a, int b, float n, int rgb) // суперЭллипс:координаты центра x,y и ширина и высота, коэф.формы, цвет. (команда Бейсик SQUIRCLE).
{
    n = n / 10;
	for(float x = 0; x < (a + a + 1); x = x + 1) {   //тут меняем шаг "x = x + 1", чем меньше шаг, тем выше четкость, но отрисовка дольше!!!

		float nReal = (float)1 / n;
		float y = pow(((1 - (pow(x, n) / pow(a, n))) * pow(b, n)), nReal);

		dot((center_x - x), (center_y + y), rgb); // левый верхний сектор
		dot((center_x + x), (center_y + y), rgb); // правый верхний сектор
		dot((center_x + x), (center_y - y), rgb); // правый нижний сектор
		dot((center_x - x), (center_y - y), rgb); // левый нижний сектор		
	}
}
	

void VGA::fillSquircle(int center_x, int center_y, int a, int b, float n, int rgb) // заполненная версия суперЭллипса: координаты центра x,y и ширина и высота, коэф.формы, цвет.(команда Бейсик SQUIRCLEC).
{
	n = n / 10;
	for(float x = 0; x < (a + a + 1); x = x + 1) {           //тут меняем шаг "x = x + 1", чем меньше шаг, тем выше четкость, но отрисовка дольше!!!

		float nReal = (float)1 / n;
		float y = pow(((1 - (pow(x, n) / pow(a, n))) * pow(b, n)), nReal);
                                                                                             // заполненная версия суперЭллипса.
		line((center_x - x), (center_y + y), (center_x - x), (center_y - y), rgb);					
		line((center_x + x), (center_y + y), (center_x + x), (center_y - y), rgb);
		
	}		
}

void VGA::drawRGBBitmap(int x,int y,uint16_t* bitmap,int w,int h){
  uint16_t * bitmap_ptr = (uint16_t *)bitmap;
  uint16_t * ptr = (uint16_t *)getDrawBuffer();
  ptr+=x;ptr+=y*_Width;
  for(int i=0;i<h;i++){
    memcpy(ptr,bitmap_ptr,w*sizeof(uint16_t));
    bitmap_ptr+=w;
    ptr+=_Width;
  }
}

void VGA::get(int x, int y) // узнаем цвет пикселя по координатам и печатаем его
{		
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    println(ptr[x+y*_Width]);
}

uint16_t VGA::getFn(int x, int y) // узнаем цвет пикселя по координатам
{		
    uint16_t *ptr = (uint16_t *)getDrawBuffer();
    return ptr[x+y*_Width];
}

uint16_t VGA::getBackFn(int x, int y) // узнаем цвет пикселя по координатам заднего буфера
{		
    uint16_t *ptr = (uint16_t *)getBackBuffer();
    return ptr[x+y*_Width];
}

unsigned short blend(unsigned short fg, unsigned short bg, unsigned char alpha)
{
    // Split foreground into components
    unsigned fg_r = fg >> 11;
    unsigned fg_g = (fg >> 5) & ((1u << 6) - 1);
    unsigned fg_b = fg & ((1u << 5) - 1);

    // Split background into components
    unsigned bg_r = bg >> 11;
    unsigned bg_g = (bg >> 5) & ((1u << 6) - 1);
    unsigned bg_b = bg & ((1u << 5) - 1);

    // Alpha blend components
    unsigned out_r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    unsigned out_g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    unsigned out_b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;

    // Pack result
    return (unsigned short) ((out_r << 11) | (out_g << 5) | out_b);
}


void VGA::dotAdd(int x, int y, int rgb1, int alpha) // в разработке
{
	uint16_t *ptr = (uint16_t *)getDrawBuffer();
	ptr[x+y*_Width] = blend(rgb1, ptr[x+y*_Width], alpha);
}

/*
// Alpha blend components
unsigned out_r = fg_r * a + bg_r * (255 - alpha);
unsigned out_g = fg_g * a + bg_g * (255 - alpha);
unsigned out_b = fg_b * a + bg_b * (255 - alpha);
out_r = (out_r + 1 + (out_r >> 8)) >> 8;
out_g = (out_g + 1 + (out_g >> 8)) >> 8;
out_b = (out_b + 1 + (out_b >> 8)) >> 8;
*/