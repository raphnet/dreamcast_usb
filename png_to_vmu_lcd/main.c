#include <stdio.h>
#include <stdint.h>

#include <png.h>

int convertPNG(FILE *fptr_in, FILE *fptr_out);

static void printusage(void)
{
	printf("Usage: ./png_to_vmu_lcd input_file output_file\n");
	printf("\n");
	printf("input_file must be a 48x32 pixels PNG file.\n");
}

int main(int argc, char **argv)
{
	FILE *fptr_in = NULL, *fptr_out = NULL;
	uint8_t header[8];
	int ret = 0;

	if (argc < 3) {
		printusage();
		return 1;
	}

	fptr_in = fopen(argv[1], "rb");
	if (!fptr_in) {
		perror("fopen");
		return 2;
	}

	if (8 != fread(header, 1, 8, fptr_in)) {
		perror("fread");
		ret = 3;
		goto done;
	}

	if (png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "Not a PNG file\n");
		ret = 3;
		goto done;
	}

	fptr_out = fopen(argv[2], "wb");
	if (!fptr_out) {
		perror("fopen outfile");
		ret = 4;
		goto done;
	}

	ret = convertPNG(fptr_in, fptr_out);

done:
	if (fptr_out) {
		fclose(fptr_out);
	}

	if (fptr_in) {
		fclose(fptr_in);
	}

	return ret;
}

int convertPNG(FILE *fptr_in, FILE *fptr_out)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	int w,h,depth,color;
	int ret;
	int x,y;
	int rotate180 = 1;
	int bpp = 1;
	unsigned char data, b;
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return -1;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return -1;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		ret = -1;
		goto done;
	}

	png_init_io(png_ptr, fptr_in);
	png_set_sig_bytes(png_ptr, 8);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA, NULL);

	w = png_get_image_width(png_ptr, info_ptr);
	h = png_get_image_height(png_ptr, info_ptr);
	depth = png_get_bit_depth(png_ptr, info_ptr);
	color = png_get_color_type(png_ptr, info_ptr);
	
	if (w != 48 || h != 32) {
		fprintf(stderr, "Image is not 48 x 32 pixels. Current size: %d x %d. Please resize or rotate your image and try again.\n", w,h);
		ret = -1;
		goto done;
	}
	
	printf("Image: %d x %d\n",w,h);
	printf("Bit depth: %d\n", depth);
	printf("Color type: %d\n", color);
	row_pointers = png_get_rows(png_ptr, info_ptr);

	switch(color)
	{
		case PNG_COLOR_TYPE_GRAY:
			printf("Processing grayscale image\n");	
			bpp = 1;
			break;
		case PNG_COLOR_TYPE_RGB:
			printf("Processing color image\n");	
			bpp = 3;
			break;
		default:
			fprintf(stderr, "Unsupported color type\n");
			ret = -1;
			goto done;
	}

	data = 0;
	b = 0x80;
	if (rotate180)
	{
		for (y=h-1; y>=0; y--) {
			for (x=w-1; x>=0; x--) {
				
				if (row_pointers[y][x*bpp]) {
					printf("XX");
				}
				else {
					printf("  ");
					data |= b;
				}

				b >>= 1;
				if (!b) {
					fprintf(fptr_out, "0x%02X, ", data);
					data = 0;
					b = 0x80;
				}
			}
			printf("\n");
			fprintf(fptr_out, "\n");
		}
	}
	else
	{
		for (y=0; y<h; y++) {
			for (x=0; x<w; x++) {
				if (row_pointers[y][x*bpp]) {
					printf("XX");
				}
				else {
					printf("  ");
					data |= b;
				}				 

				b >>= 1;
				if (!b) {
					fprintf(fptr_out, "0x%02X, ", data);
					data = 0;
					b = 0x80;
				}
			}
			printf("\n");
			fprintf(fptr_out, "\n");
		}
	}	

done:
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	return ret;
}
