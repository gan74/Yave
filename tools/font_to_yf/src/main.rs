extern crate rusttype;
extern crate rayon;

use rusttype::{FontCollection, Scale, point, Point, ScaledGlyph};
use std::fs::{File};
use std::path::{Path};
use std::io::{Read, Write, BufWriter, Result};

use std::f32;
use std::cmp;
use std::mem;
use std::slice;

use rayon::prelude::*;

const CHARACTERS: &'static str = "�ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/*-+.,;:!? ";

const RASTER_SIZE: usize = 1024;
const SDF_SIZE: usize = 64;
const EMPTY_ROW: &'static [u8] = &[0; SDF_SIZE];

const INSIDE_THRESHOLD: u8 = 127;

fn compute_sdf(pixel_data: Vec<u8>) -> Vec<u8> {
	let mut sdf_data = vec![0u8; SDF_SIZE * SDF_SIZE];

	const RATIO: usize = RASTER_SIZE / SDF_SIZE;
	const KER_HSIZE: usize = 7;

	let pix_uv = |x| x as f32 / (RASTER_SIZE - 1) as f32;
	let sdf_uv = |x| x as f32 / (SDF_SIZE - 1) as f32;
	let sqr = |x| x * x;

	let max_dist = (sqr(pix_uv((KER_HSIZE * 2 + 1) * RATIO)) * 2.0).sqrt() * 0.5;


	for x  in 0..SDF_SIZE {
		for y in 0..(SDF_SIZE - 1) {
			let is_inside = {
				let rx = x * RATIO + RATIO / 2;
				let ry = y * RATIO + RATIO / 2;
				pixel_data[ rx      +  ry      * RASTER_SIZE] as u16 + 
				pixel_data[(rx + 1) +  ry      * RASTER_SIZE] as u16 + 
				pixel_data[ rx      + (ry + 1) * RASTER_SIZE] as u16 + 
				pixel_data[(rx + 1) + (ry + 1) * RASTER_SIZE] as u16
					> (INSIDE_THRESHOLD as u16 * 4)
			};

			let dist = {
				let (u, v) = (sdf_uv(x), sdf_uv(y));
				let x_beg = cmp::max(x, KER_HSIZE) - KER_HSIZE; 
				let x_end = cmp::min(x, SDF_SIZE - (KER_HSIZE + 1)) + KER_HSIZE + 1; 
				let y_beg = cmp::max(y, KER_HSIZE) - KER_HSIZE; 
				let y_end = cmp::min(y, SDF_SIZE - (KER_HSIZE + 1)) + KER_HSIZE + 1;

				let mut sq_dist = f32::MAX;
				for rx in (x_beg * RATIO)..(x_end * RATIO) {
					for ry in (y_beg * RATIO)..(y_end * RATIO) {
						if (pixel_data[rx + ry * RASTER_SIZE] > INSIDE_THRESHOLD) != is_inside {
							let (ru, rv) = (pix_uv(rx), pix_uv(ry));
							let d = sqr(ru - u) + sqr(rv - v);
							if d < sq_dist {
								sq_dist = d;
							}
						}
					}
				}
				sq_dist.sqrt()
			};

			if dist >= max_dist {
				sdf_data[x + y * SDF_SIZE] = if is_inside { 255 } else { 0 };
				
			} else {
				let udist = ((dist / max_dist) * 127.0) as u8;
				let sdist = if is_inside { 127 + udist } else { 127 - udist };
				sdf_data[x + y * SDF_SIZE] = sdist;
			}
		}
	}

	sdf_data
}

fn raster_font(glyphs: Vec<ScaledGlyph>, offset: Point<f32>) -> Vec<Vec<u8>> {
	let mut sdfs = Vec::with_capacity(glyphs.len());
	glyphs.into_par_iter().map(|glyph| {
			let g = glyph/*.scaled(scale)*/.positioned(offset);
			let mut pixel_data = vec![0u8; RASTER_SIZE * RASTER_SIZE];
			if let Some(bb) = g.pixel_bounding_box() {
				g.draw(|x, y, v| {
						let x = x as i32 + bb.min.x;
						let y = y as i32 + bb.min.y;
						if x >= 0 && x < RASTER_SIZE as i32 && y >= 0 && y < RASTER_SIZE as i32 {
							let x = x as usize;
							let y = y as usize;
							pixel_data[x + y * RASTER_SIZE] = (v * 255.0).round() as u8;
						}
					});
			}

			compute_sdf(pixel_data)
		}).collect_into(&mut sdfs);
	sdfs
}

fn main() {
	let mut font_data = Vec::new();
	File::open("consola.ttf").expect("Unable to open file.").read_to_end(&mut font_data).unwrap();

	let font = FontCollection::from_bytes(font_data).into_font().unwrap();

	let size = RASTER_SIZE as f32;
	let scale = Scale { x: size, y: size };
	let offset = point(0.0, font.v_metrics(scale).ascent);

	let glyphs = font.glyphs_for(CHARACTERS.chars()).map(|g| g.scaled(scale)).collect::<Vec<_>>();

	let sdfs = raster_font(glyphs.clone(), offset);

	let mut file = BufWriter::new(File::create(Path::new("./font").with_extension("yt")).expect("Unable to create output file."));
	write_images(&mut file, sdfs).unwrap();
	write_font(&mut file, glyphs).unwrap();
}

fn atlas_size(char_count: usize) -> (usize, usize) {
	let atlas_width = (char_count as f32).sqrt().ceil() as usize;
	let atlas_height = char_count / atlas_width + if char_count % atlas_width == 0 { 0 } else { 1 };
	(atlas_width, atlas_height)
}


fn write_font<W: Write>(file: &mut BufWriter<W>, glyphs: Vec<ScaledGlyph>) -> Result<usize> {
	assert!(glyphs.len() == CHARACTERS.chars().count());

	let version: u32 = 1;
	let font_type: u32 = 4;
	let char_count = glyphs.len() as u32;

	file.write(b"yave")?;
	write_bin(file, &[font_type, version, char_count])?;

	let (atlas_width, atlas_height) = atlas_size(CHARACTERS.len());

	for (i, c) in CHARACTERS.chars().enumerate() {
		let ch = c as u32;

		let u = (i % atlas_width) as f32 / atlas_width as f32;
		let v = (i / atlas_width) as f32 / atlas_width as f32;
		let w = glyphs[i].h_metrics().advance_width / RASTER_SIZE as f32;
		let h = 1.0 / atlas_height as f32;

		write_bin(file, &[ch])?;
		write_bin(file, &[u, v, h, w])?;
	}

	Ok(0)
}

fn write_images<W: Write>(file: &mut BufWriter<W>, sdfs: Vec<Vec<u8>>) -> Result<usize> {
	assert!(sdfs.len() == CHARACTERS.chars().count());

	let (atlas_width, atlas_height) = atlas_size(sdfs.len());

	let image_type: u32 = 2;
	let version: u32 = 3;
	let size = ((SDF_SIZE * atlas_width) as u32, (SDF_SIZE * atlas_height) as u32);
	let layers = 1u32;
	let mipmaps = 1u32;
	let format = 9u32; // R8

	file.write(b"yave")?;
	write_bin(file, &[image_type, version, size.0, size.1, layers, mipmaps, format])?;

	for j in 0..atlas_height {
		for i in 0..atlas_width {
			let index = i + j * atlas_width;
			if let Some(c) = CHARACTERS.chars().nth(index) {
				print!("{}", c);
			}
		}
		println!("");
	}

	for j in 0..atlas_height {
		for row in 0..SDF_SIZE {
			let row_offset = row * SDF_SIZE;
			for i in 0..atlas_width {
				let index = i + j * atlas_width;

				let row_data = if index < sdfs.len() {
						&sdfs[index][row_offset..(row_offset + SDF_SIZE)]
					} else {
						EMPTY_ROW
					};

				write_bin(file, row_data)?;
			}
		}
	}

	Ok(0)
}

fn write_bin<E, T: Write>(file: &mut T, v: &[E]) -> Result<usize> {
	let slice_u8: &[u8] = unsafe {
		slice::from_raw_parts(
			v.as_ptr() as *const u8, 
			v.len() * mem::size_of::<E>()
		)
	};
	file.write(slice_u8)
}



