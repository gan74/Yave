
use std::cmp;
use std::mem;
use std::iter::*;
use image_data::*;
use rayon::prelude::*;

fn minf(a: f32, b: f32) -> f32 { if a < b { a } else { b } }
fn maxf(a: f32, b: f32) -> f32 { if a > b { a } else { b } }
fn saturate(a: f32) -> f32 { maxf(minf(a, 1.0), 0.0) }
fn fnorm(a: u8) -> f32 { a as f32 / 255.0 }
fn bnorm(a: f32) -> u8 { (saturate(a) * 255.0) as u8 }
fn to_rgb(a: &Rgba) -> Rgb { [a[0], a[1], a[2]] }
fn to_rgba(a: &Rgb) -> Rgba { [a[0], a[1], a[2], 255u8] }

const ALPHA_THRESHOLD: u8 = 128;

fn pixel_dist(a: &Rgba, b: &Rgba) -> u32 {
	fn dist_one(a: u8, b: u8) -> u32 {
		let x = a as i32 - b as i32;
		(x * x) as u32
	}
	dist_one(a[0], b[0]) + 
	dist_one(a[1], b[1]) + 
	dist_one(a[2], b[2])
}

fn pixel_dist_alpha(a: &Rgba, b: &Rgba) -> u32 {
	if a[3] < ALPHA_THRESHOLD && b[3] < ALPHA_THRESHOLD {
		0
	} else {
		pixel_dist(a, b) + 1
	}
}

fn block_dist<T: Fn(&Rgba, &Rgba) -> u32>(table: &[Rgba; 4], pixels: &Block, dist: T) -> u32 {
	let mut sum = 0u32;
	let mut max = 0u32;
	for pix in pixels.into_iter().rev() {
		let e = (0..4).into_iter().map(|x| dist(&table[x], pix)).min().unwrap();
		sum += e;
		max = cmp::max(max, e);
	}
	sum + max * 16
}










fn build_table(ends: &(Rgb, Rgb)) -> [Rgba; 4] {
	fn build_entry(ends: &(Rgb, Rgb), index: usize) -> Rgba {
		let b = if index < 2 { index * 3 } else { index - 1 } as u16;
		let a = 3 - b;
		[((b * ends.0[0] as u16 + a * ends.1[0] as u16) / 3) as u8,
		 ((b * ends.0[1] as u16 + a * ends.1[1] as u16) / 3) as u8,
		 ((b * ends.0[2] as u16 + a * ends.1[2] as u16) / 3) as u8,
		 255]
	}
	let mut interps = [[0u8; 4]; 4];
	for i in 0..4 {
		interps[i] = build_entry(ends, i);
	}
	interps
}

fn build_table_alpha(ends: &(Rgb, Rgb)) -> [Rgba; 4] {
	let mut mid = [0u8; 3];
	for i in 0..3 {
		mid[i] = (((ends.0[i] as u16) + (ends.1[i] as u16)) / 2) as u8;
	}
	[to_rgba(&ends.0), to_rgba(&ends.1), to_rgba(&mid), [0u8; 4]]
}

fn minmax(pixels: &Block) -> (Rgb, Rgb) {
	let mut min = [255u8; 3];
	let mut max = [0u8; 3];
	for p in pixels.into_iter() {
		for i in 0..3 {
			min[i] = cmp::min(min[i], p[i]);
			max[i] = cmp::max(max[i], p[i]);
		}
	}
	(min, max)
}


fn quantize_endpoints(ends: &(Rgb, Rgb)) -> (Rgb, Rgb) {
	fn quantize(end: &Rgb) -> Rgb {
		//[end[0] & 0xF8, end[1] & 0xFC, end[2] & 0xF8]
		let bits = [3, 2, 3];
		let mut e = *end;
		for i in 0..3 {
			let q = (e[i] >> bits[i]) << bits[i];
			e[i] = q + ((e[i] >> (bits[i] - 1)) & 0x01);
		}
		e
	}
	(quantize(&ends.0), quantize(&ends.1))
}

fn extrapolate_endpoints(ends: &(Rgb, Rgb)) -> (Rgb, Rgb) {
	let mut a = ends.0;
	let mut b = ends.1;
	for i in 0..3 {
		let f = (fnorm(ends.0[i]) - fnorm(ends.1[i])) * 1.5;
		a[i] = bnorm(fnorm(ends.0[i]) - f);
		b[i] = bnorm(fnorm(ends.1[i]) + f);
	}
	(a, b)
}

fn endpoints_sorted(ends: &(Rgb, Rgb)) -> bool {
	encode_endpoint(&ends.0) < encode_endpoint(&ends.1)
}

fn sort_endpoints(ends: &(Rgb, Rgb)) -> (Rgb, Rgb) {
	if endpoints_sorted(ends) {
		*ends
	} else {
		(ends.1, ends.0)
	}
}

fn sort_quantize_endpoints(ends: &(Rgb, Rgb)) -> (Rgb, Rgb) {
	quantize_endpoints(&sort_endpoints(ends))
}

fn build_endpoints(pixels: &Block, quality: u8) -> Vec<(Rgb, Rgb)> {
	let (min, max) = quantize_endpoints(&minmax(pixels));

	let mut out = Vec::new();
	out.push((min, max));
	
	if quality > 0 && min != max {
		if quality == 255 {
			for i in 0..3 {
				let mid = ((min[i] as u16 + max[i] as u16) / 2) as u8;
				for o in (min[i] + 1)..mid {
					let mut min = min;
					min[i] = o;
					
					for i in 0..3 {
						for o in mid..max[i] {
							let mut max = max;
							max[i] = o;
							
							out.push((min, max));
						}
					}
				}
			}
		} else {
			for i in 0..(BLOCK_PIXELS - 1) {
				for j in (i + 1)..BLOCK_PIXELS {
				
					let px = (to_rgb(&pixels[i]), to_rgb(&pixels[j]));
					let qpx = sort_quantize_endpoints(&px);
					out.push(qpx);
					
					if quality > 1 {
						let extrapolated = sort_quantize_endpoints(&extrapolate_endpoints(&px));
						out.push(extrapolated);
						
						if quality > 2 {
							let e = (extrapolated.0, max);
							if endpoints_sorted(&e) {
								out.push(e);
							}
							
							let e = (min, extrapolated.1);
							if endpoints_sorted(&e) {
								out.push(e);
							}
							
							if quality > 3 {
								out.push(sort_endpoints(&(extrapolated.0, qpx.1)));
								out.push(sort_endpoints(&(qpx.0, extrapolated.1)));
							}
						}
					}
				}
			}
		}
	}
	
	out
}









fn encode_endpoint(end: &Rgb) -> u16 {
	((end[0] as u16 >> 3) << 11) |
	((end[1] as u16 >> 2) << 5) |
	(end[2] as u16 >> 3)
}

fn encode_pixels<T: Fn(&Rgba, &Rgba) -> u32>(table: &[Rgba; 4], pixels: &Block, dist: T) -> u32 {
	let mut mask = 0u32;
	for pix in pixels.into_iter().rev() {
		let best = (0..4).into_iter().min_by_key(|x| dist(&table[*x], pix)).unwrap() as u32;
		mask = (mask << 2) | (best & 0x03);
	}
	mask
}

fn encode_block(pixels: &Block, quality: u8) -> u64 {
	let alpha = pixels.into_iter().any(|p| p[3] < ALPHA_THRESHOLD);
	
	if alpha {
		let (ends, table) = 
			build_endpoints(pixels, quality).iter()
			.map(|ends| (*ends, build_table_alpha(ends)))
			.min_by_key(|tbl| block_dist(&tbl.1, pixels, pixel_dist_alpha)).unwrap();
			
		let min = encode_endpoint(&ends.1) as u64;
		let max = encode_endpoint(&ends.0) as u64;

		let encoded = encode_pixels(&table, &pixels, pixel_dist_alpha) as u64;
		(encoded << 32) | (min << 16) | max
		
	} else {
		let (ends, table) = 
			build_endpoints(pixels, quality).iter()
			.map(|ends| (*ends, build_table(ends)))
			.min_by_key(|tbl| block_dist(&tbl.1, pixels, pixel_dist)).unwrap();
			
		let min = encode_endpoint(&ends.0) as u64;
		let max = encode_endpoint(&ends.1) as u64;

		let encoded = encode_pixels(&table, &pixels, pixel_dist) as u64;
		(encoded << 32) | (min << 16) | max
	}
}




pub fn encode(image: &ImageData, quality: u8) -> Result<Vec<u8>, ()> {
	if image.size.0 % BLOCK_SIZE != 0 || image.size.1 % BLOCK_SIZE != 0 {
		return Err(());
	}
	
	let blocks = image.blocks().count();
	
	let mut out = Vec::with_capacity(blocks);
	(0..blocks).into_par_iter()
		.map(|i| {
			encode_block(&image.blocks().nth(i).unwrap(), quality)
		}).collect_into_vec(&mut out);
		
	unsafe {
		let mut out: Vec<u8> = mem::transmute(out);
		out.set_len(blocks * 8);
		Ok(out)
	}
}
