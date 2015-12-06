/*
 * TimeShifter: given a set of standalone frames, creates a sequence of them shifted in time.
 * Copyright (C) 2015 Argel Arias <levhita@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

struct png_image {
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep * row_pointers;
};


struct png_image *read_png_file(char* file_name) {
	//printf("Opening '%s'<\n", file_name);
	struct png_image *image =malloc(sizeof(struct png_image));
	int width, height, y;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep * row_pointers; 

	// 8 is the maximum size that can be checked
	char header[8];  

	// open file and test for it being a png
	FILE *fp = fopen(file_name, "rb");
	if (!fp){
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	}
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8)){
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);
	}


	// initialize stuff
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr){
		abort_("[read_png_file] png_create_read_struct failed");
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr){
		abort_("[read_png_file] png_create_info_struct failed");
	}

	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[read_png_file] Error during init_io");
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);


	// read file
	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[read_png_file] Error during read_image");
	}

	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++){
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
	}

	png_read_image(png_ptr, row_pointers);

	fclose(fp);

	//Return the image in a nice structure
 	image->width=width;
	image->height=height;
	image->color_type=color_type;
	image->bit_depth=bit_depth;
	image->png_ptr=png_ptr;
	image->info_ptr=info_ptr;
	image->row_pointers=row_pointers; 

	return image;
}


void write_png_file(struct png_image *image, char* file_name) {
	//printf("Writing %s\n", file_name);
	png_structp png_ptr;
	png_infop info_ptr;
	
	// create file
	FILE *fp = fopen(file_name, "wb");
	if (!fp)
		abort_("[write_png_file] File %s could not be opened for writing", file_name);


	// initialize stuff
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr){
		abort_("[write_png_file] png_create_write_struct failed");
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr){
		abort_("[write_png_file] png_create_info_struct failed");
	}

	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[write_png_file] Error during init_io");
	}

	png_init_io(png_ptr, fp);


	// write header
	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[write_png_file] Error during writing header");
	}

	png_set_IHDR(png_ptr, info_ptr, image->width, image->height,
		image->bit_depth, image->color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	//Write bytes
	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[write_png_file] Error during writing bytes");
	}

	// Actually writes the image
	png_write_image(png_ptr, image->row_pointers);

	//End write
	if (setjmp(png_jmpbuf(png_ptr))){
		abort_("[write_png_file] Error during end of write");
	}

	png_write_end(png_ptr, NULL);

	fclose(fp);
}

void free_image(struct png_image *image){
	int y;
	// cleanup heap allocation
	for (y=0; y<image->height; y++){
		free(image->row_pointers[y]);
	}
	free(image->row_pointers);
	free(image->info_ptr);
	free(image->png_ptr);
}

void copy_slice(struct png_image *frame_image, struct png_image *slice_image, int start, int height) {
	int x,y;
	//printf(" Slicing start: %d, height, %d\n", start, height);
	for (y=start; y<(start+height); y++) {
		png_byte* frame_row = frame_image->row_pointers[y];
		png_byte* slice_row = slice_image->row_pointers[y];
		for (x=0; x<frame_image->width; x++) {
			png_byte* frame_ptr = &(frame_row[x*4]);
			png_byte* slice_ptr = &(slice_row[x*4]);
			slice_ptr[0] = frame_ptr[0];
			slice_ptr[1] = frame_ptr[1];
			slice_ptr[2] = frame_ptr[2];
			slice_ptr[3] = frame_ptr[3];
		}
	}
}

int main(int argc, char **argv) {
	struct png_image *frame_image;
	int slices, slice, shifted_slice, frames, frame, slice_height,i;
	char source[100];
	char output[100];
	char frame_filename[150];
	char slice_filename[150];
	
	if (argc != 5){
		abort_("Usage: program_name <source> <output> <frames> <slices>");
	}
	
	strcpy(source, argv[1]);
	strcpy(output, argv[2]);
	frames  = strtol(argv[3], NULL, 10);
	slices  = strtol(argv[4], NULL, 10);

	// A FOR cycle would be much better, but we need to load the first image to
	// make some validations and get some number for the rest of the process.
	frame=0;
	sprintf(frame_filename, "%s/%03i.png", source, frame);
	frame_image = read_png_file(frame_filename);
	slice_height = (frame_image->height/slices);
		
	if (slices > frame_image->height) {
		abort_("[slice_png] Slices can't be larger than height");
	}
	

	if (png_get_color_type(frame_image->png_ptr, frame_image->info_ptr) == PNG_COLOR_TYPE_RGB) {
		abort_("[slice_png] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
			"(lacks the alpha channel)");
	}

	if (png_get_color_type(frame_image->png_ptr, frame_image->info_ptr) != PNG_COLOR_TYPE_RGBA){
		abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
			PNG_COLOR_TYPE_RGBA, png_get_color_type(frame_image->png_ptr, frame_image->info_ptr));
	}

	struct png_image *slice_images[frames];
	for(i=0;i<frames;i++){
		sprintf(slice_filename, "%s/%03i.png", output, i);
		slice_images[i] = read_png_file(slice_filename);
	}
	do {
		//printf("Frame %03d\n", frame);
		for(slice=0; slice<slices; slice++){
			shifted_slice = slice+frame;
			while (shifted_slice>=frames){
				shifted_slice-=frames;
			}
			//printf("%03d Shifted: %03d | Start: %03dpx\n", slice, shifted_slice, shifted_slice*slice_height);
			copy_slice(frame_image, slice_images[shifted_slice], slice*slice_height, slice_height);
		}                 
		free_image(frame_image);

		if (++frame<frames) {
			sprintf(frame_filename, "%s/%03i.png", source, frame);
			frame_image = read_png_file(frame_filename);
		}
	} while(frame < frames);

	printf("Saving Timeshifted Frames...\n");
	for(i=0;i<frames;i++){
		sprintf(slice_filename, "%s/%03i.png", output, i);
		write_png_file(slice_images[i], slice_filename);
		free_image(slice_images[i]);
	}
	return 0;
}
