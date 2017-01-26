
use std::cmp;

fn is_blk(size: (usize, usize)) -> bool {
	return size.0 % 4 == 0 && size.1 % 4 == 0;
}

fn bc1_encode_endpoint(pix: &[i16; 4]) -> i16 {
	((pix[0] >> 3) << 11) |
	((pix[1] >> 2) << 5) |
	(pix[2] >> 3)
}

fn bc1_interp(min: &[i16; 4], max: &[i16; 4], index: usize) -> [u8; 4] {
	let coefs = match index {
		0x0 => (3, 0),
		0x1 => (0, 3),
		0x2 => (2 ,1),
		_ => (1, 2)
	};
	[((coefs.1 * min[0] + coefs.0 * max[0]) / 3) as u8,
	 ((coefs.1 * min[1] + coefs.0 * max[1]) / 3) as u8,
	 ((coefs.1 * min[2] + coefs.0 * max[2]) / 3) as u8,
	 ((coefs.1 * min[3] + coefs.0 * max[3]) / 3) as u8]
}

fn bc1_table(min: &[i16; 4], max: &[i16; 4]) -> [[u8; 4]; 4] {
	let mut interps = [[0u8; 4]; 4];
	for i in 0..4 {
		interps[i] = bc1_interp(&min, &max, i);
	}
	interps
}

fn bc1_minmax(pixels: &[[u8; 4]; 16]) -> ([i16; 4], [i16; 4]) {
	let mut min = [255i16; 4];
	let mut max = [0i16; 4];
	for p in pixels.into_iter() {
		for i in 0..4 {
			min[i] = cmp::min(min[i], p[i] as i16);
			max[i] = cmp::max(max[i], p[i] as i16);
		}
	}
	(min, max)
}

fn bc1_dist_one(a: i16, b: i16) -> u32 {
	(a - b).abs() as u32
}

fn bc1_dist(a: &[u8; 4], b: &[u8; 4]) -> u32 {
	bc1_dist_one(a[0] as i16, b[0] as i16) + 
	bc1_dist_one(a[1] as i16, b[1] as i16) + 
	bc1_dist_one(a[2] as i16, b[2] as i16) + 
	bc1_dist_one(a[3] as i16, b[3] as i16)
}

fn bc1_encode(table: &[[u8; 4]; 4], pixels: &[[u8; 4]; 16]) -> u32 {
	let mut mask = 0u32;
	for pix in pixels.into_iter().rev() {
		let best = (0..4).into_iter().min_by_key(|x| bc1_dist(&table[*x], pix)).unwrap() as u32;
		mask = (mask << 2) | (best & 0x03);
	}
	mask
}


fn bc1_total_dist(table: &[[u8; 4]; 4], pixels: &[[u8; 4]; 16]) -> u32 {
	let mut sum = 0u32;
	for pix in pixels.into_iter().rev() {
		sum += (0..4).into_iter().map(|x| bc1_dist(&table[x], pix)).min().unwrap();
	}
	sum
}

fn iter_endpoint(mut to_iter: [i16; 4], other: &[i16; 4], pixels: &[[u8; 4]; 16], offsets: &[i16]) -> [i16; 4] {
	let (mut best, mut index, mut offset) = (bc1_total_dist(&bc1_table(&to_iter, other), pixels), 0, 0);
	for i in 0..3 {
		for o in offsets.into_iter() {
			let mut x = to_iter;
			x[i] += *o;
			let table = bc1_table(&x, &other);
			let dist = bc1_total_dist(&table, pixels);
			if dist < best {
				best = dist;
				index = i;
				offset = *o;
			}
		}
	}
	to_iter[index] += offset;
	to_iter
}


pub fn bc1(image: &Vec<u8>, size: (usize, usize), quality: u8) -> Result<Vec<u8>, ()> {
	if !is_blk(size) {
		return Err(());
	}

    let mut out = Vec::with_capacity(image.len() / 8);

   	//let mut total_distance = 0u64;

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

   			let (mut min, mut max) = bc1_minmax(&pixels);

   			for _ in 0..quality {
   				min = iter_endpoint(min, &max, &pixels, &[2, 4, 8]);
   				max = iter_endpoint(max, &min, &pixels, &[-2, -4, -8]);
   			}

   			let table = bc1_table(&min, &max);

   			//total_distance += bc1_total_dist(&table, &pixels) as u64;

   			// if we want alpha swap min/max (or encode min first)
			let min = bc1_encode_endpoint(&min);
			let max = bc1_encode_endpoint(&max);

			out.push((max & 0xFF) as u8);
			out.push((max >> 8) as u8);

			out.push((min & 0xFF) as u8);
			out.push((min >> 8) as u8);

			let mut encoded = bc1_encode(&table, &pixels);
			for _ in 0..4 {
				out.push((encoded & 0xFF) as u8);
				encoded >>= 8;
			}
		}
	}
	//println!("total = {}\nper-pixel = {}", total_distance, total_distance as f64/(size.0 * size.1) as f64);
	Ok(out)
}
