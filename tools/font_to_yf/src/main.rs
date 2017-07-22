extern crate rusttype;
extern crate rayon;

use rusttype::{FontCollection, Scale, point};
use std::fs::{File};
use std::io::{Read, Write};

use std::f32;
use std::cmp;

use rayon::prelude::*;

const CHARACTERS: &'static str = "�abcdefghijklmnopqrstuvwxyz0123456789/*-+.,;:!? ";

const RASTER_SIZE: usize = 1024;
const SDF_SIZE: usize = 64;

const INSIDE_THRESHOLD: u8 = 127;

fn compute_sdf(pixel_data: Vec<u8>) -> Vec<u8> {
	let mut sdf_data = vec![0u8; SDF_SIZE * SDF_SIZE];

	const RATIO: usize = RASTER_SIZE / SDF_SIZE;
	const KER_HSIZE: usize = 7;

	let pix_uv = |x| x as f32 / (RASTER_SIZE - 1) as f32;
	let sdf_uv = |x| x as f32 / (SDF_SIZE - 1) as f32;
	let sqr = |x| x * x;

	let max_dist = {
		let ker_size = (KER_HSIZE * 2 + 1) * RATIO;
		let uv_size = pix_uv(ker_size);
		(sqr(uv_size) + sqr(uv_size)).sqrt() * 0.5
	};


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

	for mut s in &mut sdf_data {
		let c = match *s {
			0...50 => b' ',
			50...90 => b'.',
			90...127 => b'-',
			127...191 => b'+',
			_ => b'#'
		};
		*s = c;
	}

	sdf_data
}


fn main() {
	let mut font_data = Vec::new();
	File::open("consola.ttf").expect("Unable to open file.").read_to_end(&mut font_data).unwrap();

	let font = FontCollection::from_bytes(font_data).into_font().unwrap();

	let size = RASTER_SIZE as f32;
	let scale = Scale { x: size, y: size };

	let glyphs = font.glyphs_for(CHARACTERS.chars()).collect::<Vec<_>>();
	let offset = point(0.0, font.v_metrics(scale).ascent);

	let mut sdf_images = Vec::with_capacity(glyphs.len());
	glyphs.into_par_iter().map(|glyph| {
			let g = glyph.scaled(scale).positioned(offset);
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
		}).collect_into(&mut sdf_images);



	let stdout = ::std::io::stdout();
	let mut handle = stdout.lock(); 
	for img in sdf_images {
		assert_eq!(SDF_SIZE * SDF_SIZE, img.len());
		for j in 0..SDF_SIZE {
			handle.write(&img[(j * SDF_SIZE)..((j + 1) * SDF_SIZE)]).unwrap();
			handle.write(b"\n").unwrap();
		}
		handle.write(b"\n").unwrap();
	}
}