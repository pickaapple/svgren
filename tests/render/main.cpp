#include "../../src/svgren/render.hpp"

#include <utki/debug.hpp>
#include <utki/config.hpp>

#include <papki/FSFile.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#if M_OS == M_OS_LINUX
#	include <X11/Xlib.h>
#endif


#if M_OS == M_OS_LINUX
void processEvent(Display *display, Window window, XImage *ximage, int width, int height){
	XEvent ev;
	XNextEvent(display, &ev);
	switch(ev.type)
	{
	case Expose:
		XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 1, 1, width, height);
		break;
	case ButtonPress:
		exit(0);
	}
}
#endif



int writePng(const char* filename, int width, int height, std::uint32_t *buffer)
{
   int code = 0;
   FILE *fp = NULL;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   
   // Open file for writing (binary mode)
   fp = fopen(filename, "wb");
   if (fp == NULL) {
      fprintf(stderr, "Could not open file %s for writing\n", filename);
      code = 1;
      throw utki::Exc("Could not open file for writing");
   }
   
   // Initialize write structure
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      fprintf(stderr, "Could not allocate write struct\n");
      code = 1;
      throw utki::Exc("Could not allocate write struct");
   }

   // Initialize info structure
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fprintf(stderr, "Could not allocate info struct\n");
      code = 1;
      throw utki::Exc("Could not allocate info struct");
   }
 
   png_init_io(png_ptr, fp);

   // Write header (8 bit colour depth)
   png_set_IHDR(png_ptr, info_ptr, width, height,
         8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   // Set title
//   if (title != NULL) {
//      png_text title_text;
//      title_text.compression = PNG_TEXT_COMPRESSION_NONE;
//      title_text.key = "Title";
//      title_text.text = title;
//      png_set_text(png_ptr, info_ptr, &title_text, 1);
//   }

   png_write_info(png_ptr, info_ptr);
   
   // Allocate memory for one row (4 bytes per pixel - RGBA)
   //row = (png_bytep) malloc(4 * width * sizeof(png_byte));

   // Write image data
   int y;
   auto p = buffer;
   for (y=0 ; y<height ; y++, p += width) {
//      for (x=0 ; x<width ; x++) {
//         setRGB(&(row[x*3]), buffer[y*width + x]);
//      }
      png_write_row(png_ptr, reinterpret_cast<png_bytep>(p));
   }

   // End write
   png_write_end(png_ptr, NULL);
 
   if (fp != NULL) fclose(fp);
   if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
   if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
   //if (row != NULL) free(row);

   return code;
}


int main(int argc, char **argv){
	if(argc != 3){
		std::cout << "Error: 2 arguments expected: <in-svg-file> <out-png-file>" << std::endl;
		return 1;
	}
	
	std::string filename = argv[1];
	
	auto dom = svgdom::load(papki::FSFile(filename));
//	auto dom = svgdom::load(papki::FSFile("tiger.svg"));
	
	ASSERT_ALWAYS(dom)
	
	unsigned imWidth = 0;
	unsigned imHeight = 0;
	auto img = svgren::render(*dom, imWidth, imHeight, 96, false);
	
	TRACE(<< "imWidth = " << imWidth << " imHeight = " << imHeight << " img.size() = " << img.size() << std::endl)

	writePng(argv[2], imWidth, imHeight, &*img.begin());
//	{
//		papki::FSFile file("out.data");
//		papki::File::Guard guard(file, papki::File::E_Mode::CREATE);
//		file.write(utki::Buf<std::uint8_t>(reinterpret_cast<std::uint8_t*>(&*img.begin()), img.size() * 4));
//	}

	
#if M_OS == M_OS_LINUX
	for(auto& c : img){
		c = (c & 0xff00ff00) | ((c << 16) & 0xff0000) | ((c >> 16) & 0xff);
	}
	
	XImage *ximage;
	
	int width = 800, height=800;

	Display *display = XOpenDisplay(NULL);
	
	Visual *visual = DefaultVisual(display, 0);
	
	Window window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, width, height, 1, 0, 0);
	
	if(visual->c_class != TrueColor){
		TRACE_ALWAYS(<< "Cannot handle non true color visual ...\n" << std::endl)
		return 1;
	}

	ximage = XCreateImage(display, visual, 24, ZPixmap, 0, reinterpret_cast<char*>(&*img.begin()), imWidth, imHeight, 8, 0);
	
	XSelectInput(display, window, ButtonPressMask|ExposureMask);
	
	XMapWindow(display, window);
	
	while(true){
		processEvent(display, window, ximage, imWidth, imHeight);
	}
#endif

	TRACE_ALWAYS(<< "[PASSED]" << std::endl)
}
