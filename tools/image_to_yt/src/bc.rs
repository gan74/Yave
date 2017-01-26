
use std::cmp;

type Rgba = [u8; 4];
/*type Hsva = [u8; 4];


fn minf(a: f32, b: f32) -> f32 { if a < b { a } else { b } }
fn maxf(a: f32, b: f32) -> f32 { if a > b { a } else { b } }
fn fnorm(a: u8) -> f32 { a as f32 / 255.0 }
fn bnorm(a: f32) -> u8 { (a * 255.0) as u8 }

fn to_hsva(rgba: &Rgba) -> Hsva {
	let (r, g, b) = (fnorm(rgba[0]), fnorm(rgba[1]), fnorm(rgba[2]));
	let max = maxf(maxf(r , g), b);
	let min = minf(minf(r , g), b);
	let c = max - min;
	let h = 
		if c == 0.0 {
			0.0
		} else {
			(if max == r {
				(g - b) / c
			} else if max == g {
				(b - r) / c + 2.0
			} else {
				(r - g) / c + 4.0
			}) / 6.0
		};

	let s = if max == 0.0 { 0.0 } else { c / max };
	[bnorm(h), bnorm(s), bnorm(max), rgba[3]]
}

fn to_rgba(hsva: &Hsva) -> Rgba {
	let v = fnorm(hsva[2]);
	let c = v * fnorm(hsva[3]);
	let h = fnorm(hsva[0]) * 6.0;
	let x = c * (1.0 - (h % 2.0 - 1.0).abs());
	let rgb1 =
		if h < 1.0 {
			(c, x, 0.0)
		} else if h < 2.0 {
			(x, c, 0.0)
		} else if h < 3.0 {
			(0.0, c, x)
		} else if h < 4.0 {
			(0.0, x, c)
		} else if h < 5.0 {
			(x, 0.0, c)
		} else {
			(c, 0.0, x)
		};
	let m = v - c;
	[bnorm(rgb1.0 + m),
	bnorm(rgb1.1 + m),
	bnorm(rgb1.2 + m),
	hsva[3]]
}*/




fn bc1_dist(a: &Rgba, b: &Rgba) -> u32 {
	fn bc1_dist_one(a: u8, b: u8) -> u32 {
		let x = a as i32 - b as i32;
		(x * x) as u32
	}
	bc1_dist_one(a[0], b[0]) + 
	bc1_dist_one(a[1], b[1]) + 
	bc1_dist_one(a[2], b[2]) + 
	bc1_dist_one(a[3], b[3])
}

fn bc1_total_dist(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut sum = 0u32;
	for pix in pixels.into_iter().rev() {
		sum += (0..4).into_iter().map(|x| bc1_dist(&table[x], pix)).min().unwrap();
	}
	sum
}







fn bc1_build_entry(min: &Rgba, max: &Rgba, index: usize) -> Rgba {
	let coefs = match index {
		0x0 => (3, 0),
		0x1 => (0, 3),
		0x2 => (2 ,1),
		_ => (1, 2)
	};
	[((coefs.1 * min[0] as u16 + coefs.0 * max[0] as u16) / 3) as u8,
	 ((coefs.1 * min[1] as u16 + coefs.0 * max[1] as u16) / 3) as u8,
	 ((coefs.1 * min[2] as u16 + coefs.0 * max[2] as u16) / 3) as u8,
	 ((coefs.1 * min[3] as u16 + coefs.0 * max[3] as u16) / 3) as u8]
}


fn bc1_build_table(min: &Rgba, max: &Rgba) -> [Rgba; 4] {
	let mut interps = [[0u8; 4]; 4];
	for i in 0..4 {
		interps[i] = bc1_build_entry(&min, &max, i);
	}
	interps
}

fn bc1_minmax(pixels: &[Rgba; 16]) -> (Rgba, Rgba) {
	let mut min = [u8::max_value(); 4];
	let mut max = [0u8; 4];
	for p in pixels.into_iter() {
		for i in 0..4 {
			min[i] = cmp::min(min[i], p[i]);
			max[i] = cmp::max(max[i], p[i]);
		}
	}
	(min, max)
}

fn bc1_build_endpoints(pixels: &[Rgba; 16]) -> [(Rgba, Rgba); 1] {
	let (min, max) = bc1_minmax(pixels);

	let out = [(min, max)];

	//println!("{:?}", to_rgba(&to_rgba(&[0, 0, 0, 255])));
	/*for i in 1..out.len() {
		let offset = 4 * i as u8;
		for c in 0..3 {
			if out[i].0[c] < 128 {
				out[i].0[c] += offset;
			}
			if out[i].1[c] > 128 {
				out[i].1[c] -= offset;
			}
		}
	}*/

	out
}








fn bc1_encode_endpoint(pix: &Rgba) -> u16 {
	((pix[0] as u16 >> 3) << 11) |
	((pix[1] as u16 >> 2) << 5) |
	(pix[2] as u16 >> 3)
}

fn bc1_encode_pixel(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut mask = 0u32;
	for pix in pixels.into_iter().rev() {
		let best = (0..4).into_iter().min_by_key(|x| bc1_dist(&table[*x], pix)).unwrap() as u32;
		mask = (mask << 2) | (best & 0x03);
	}
	mask
}

fn bc1_encode_block(pixels: &[Rgba; 16]) -> (u64, u32) {
	/*let (min, max) = bc1_build_endpoints(&pixels)[0];
	let table = bc1_build_table(&min, &max);*/

	let ((min, max), table) = bc1_build_endpoints(pixels).into_iter()
		.map(|pts| (*pts, bc1_build_table(&pts.0, &pts.1)))
		.min_by_key(|tbl| bc1_total_dist(&tbl.1, pixels)).unwrap();

	let dist = bc1_total_dist(&table, pixels);

	let min = bc1_encode_endpoint(&min) as u64;
	let max = bc1_encode_endpoint(&max) as u64;

	let encoded = bc1_encode_pixel(&table, &pixels) as u64;

	((encoded << 32) | (min << 16) | max, dist)
}






pub fn bc1(image: &Vec<u8>, size: (usize, usize), _: u8) -> Result<Vec<u8>, ()> {
	if !(size.0 % 4 == 0 && size.1 % 4 == 0) {
		return Err(());
	}

    let mut out = Vec::with_capacity(image.len() / 8);
    let mut total_dist = 0u32;

    let block_size = 4;
    let blk_count = (size.0 / block_size, size.1 / block_size);

   	for x in 0..blk_count.0 {
   		for y in 0..blk_count.1 {

   			let mut pixels = [[0u8; 4]; 16]; // No compile time constants
   			for px in 0..block_size {
   				for py in 0..block_size {
   					let x = x * block_size + px;
   					let y = y * block_size + py;

   					let pi = (x * size.0 + y) * 4; // in bytes
   					let pix = [image[pi], image[pi + 1], image[pi + 2], image[pi + 3]];

   					pixels[px * block_size + py] = pix;
   				}
   			}

   			let (mut encoded, dist) = bc1_encode_block(&pixels);
   			total_dist += dist;

   			for _ in 0..8 {
   				out.push((encoded & 0xFF) as u8);
   				encoded >>= 8;
   			}
		}
	}
	println!("per-pixel dist = {:?}", total_dist / (size.0 * size.1) as u32);
	Ok(out)
}
