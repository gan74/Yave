
use std::cmp;

type Rgba = [u8; 4];

fn is_blk(size: (usize, usize)) -> bool {
	return size.0 % 4 == 0 && size.1 % 4 == 0;
}

fn bc1_encode_endpoint(pix: &Rgba) -> u16 {
	((pix[0] as u16 >> 3) << 11) |
	((pix[1] as u16 >> 2) << 5) |
	(pix[2] as u16 >> 3)
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

fn bc1_build_endpoints(pixels: &[Rgba; 16]) -> (Rgba, Rgba) {
	let mut min = [u8::max_value(); 4];
	let mut max = [0u8; 4];
	//let mut avg = [0f32; 4];
	for p in pixels.into_iter() {
		for i in 0..4 {
			min[i] = cmp::min(min[i], p[i]);
			max[i] = cmp::max(max[i], p[i]);
			//avg[i] += p[i] as f32;
		}
	}
	(min, max)

	/*let max_len = ((max[0] * max[0] + max[1] * max[1] + max[2] * max[2]) as f32).sqrt();
	let min_len = ((min[0] * min[0] + min[1] * min[1] + min[2] * min[2]) as f32).sqrt();
	let avg_len = (avg[0] * avg[0] + avg[1] * avg[1] + avg[2] * avg[2]).sqrt();
	let max_len = avg_len / max_len;
	let norm_max_avg = [(avg[0] / max_len) as u8, (avg[1] / max_len) as u8, (avg[2] / max_len) as u8, max[3]];
	let min_len = avg_len / min_len;
	let norm_min_avg = [(avg[0] / min_len) as u8, (avg[1] / min_len) as u8, (avg[2] / min_len) as u8, min[3]];

	let min_point = *pixels.into_iter().min_by_key(|p| bc1_dist(&p, &min)).unwrap();
	let max_point = *pixels.into_iter().min_by_key(|p| bc1_dist(&p, &max)).unwrap();

	[(min, max), 
	(norm_min_avg, norm_max_avg), 
	(min, norm_max_avg), 
	(norm_min_avg, max),

	(min_point, max_point), 
	(min, max_point), 
	(min_point, max)]*/
}

fn bc1_dist_one(a: u8, b: u8) -> u32 {
	let x = a as i32 - b as i32;
	(x * x) as u32
}

fn bc1_dist(a: &Rgba, b: &Rgba) -> u32 {
	bc1_dist_one(a[0], b[0]) + 
	bc1_dist_one(a[1], b[1]) + 
	bc1_dist_one(a[2], b[2]) + 
	bc1_dist_one(a[3], b[3])
}

fn bc1_encode_pixel(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut mask = 0u32;
	for pix in pixels.into_iter().rev() {
		let best = (0..4).into_iter().min_by_key(|x| bc1_dist(&table[*x], pix)).unwrap() as u32;
		mask = (mask << 2) | (best & 0x03);
	}
	mask
}

fn bc1_total_dist(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut sum = 0u32;
	for pix in pixels.into_iter().rev() {
		sum += (0..4).into_iter().map(|x| bc1_dist(&table[x], pix)).min().unwrap();
	}
	sum
}

fn bc1_encode_block(pixels: &[Rgba; 16]) -> (u64, u32) {
	let (min, max) = bc1_build_endpoints(&pixels);

	//let (min, max) = *bc1_build_endpoints(pixels).into_iter().min_by_key(|p| bc1_total_dist(&bc1_build_table(&p.0, &p.1), pixels)).unwrap();

	let table = bc1_build_table(&min, &max);

	let dist = bc1_total_dist(&table, pixels);

	let min = bc1_encode_endpoint(&min) as u64;
	let max = bc1_encode_endpoint(&max) as u64;

	let encoded = bc1_encode_pixel(&table, &pixels) as u64;

	((encoded << 32) | (min << 16) | max, dist)
}

pub fn bc1(image: &Vec<u8>, size: (usize, usize), _: u8) -> Result<Vec<u8>, ()> {
	if !is_blk(size) {
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
