
use std::cmp;

type Endpoint = [u16; 4];
type Rgba = [u8; 4];

fn is_blk(size: (usize, usize)) -> bool {
	return size.0 % 4 == 0 && size.1 % 4 == 0;
}

fn bc1_encode_endpoint(pix: &Endpoint) -> u16 {
	((pix[0] >> 3) << 11) |
	((pix[1] >> 2) << 5) |
	(pix[2] >> 3)
}

fn bc1_build_entry(min: &Endpoint, max: &Endpoint, index: usize) -> Rgba {
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

fn bc1_build_table(min: &Endpoint, max: &Endpoint) -> [Rgba; 4] {
	let mut interps = [[0u8; 4]; 4];
	for i in 0..4 {
		interps[i] = bc1_build_entry(&min, &max, i);
	}
	interps
}

fn bc1_minmax(pixels: &[Rgba; 16]) -> (Endpoint, Endpoint) {
	let mut min = [256u16; 4];
	let mut max = [0u16; 4];
	for p in pixels.into_iter() {
		for i in 0..4 {
			min[i] = cmp::min(min[i], p[i] as u16);
			max[i] = cmp::max(max[i], p[i] as u16);
		}
	}
	(min, max)
}

fn bc1_dist_one(a: i16, b: i16) -> u32 {
	(a - b).abs() as u32
}

fn bc1_dist(a: &Rgba, b: &Rgba) -> u32 {
	bc1_dist_one(a[0] as i16, b[0] as i16) + 
	bc1_dist_one(a[1] as i16, b[1] as i16) + 
	bc1_dist_one(a[2] as i16, b[2] as i16) + 
	bc1_dist_one(a[3] as i16, b[3] as i16)
}

fn bc1_encode_pixel(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut mask = 0u32;
	for pix in pixels.into_iter().rev() {
		let best = (0..4).into_iter().min_by_key(|x| bc1_dist(&table[*x], pix)).unwrap() as u32;
		mask = (mask << 2) | (best & 0x03);
	}
	mask
}

/*fn bc1_total_dist(table: &[Rgba; 4], pixels: &[Rgba; 16]) -> u32 {
	let mut sum = 0u32;
	for pix in pixels.into_iter().rev() {
		sum += (0..4).into_iter().map(|x| bc1_dist(&table[x], pix)).min().unwrap();
	}
	sum
}

fn iter_endpoint(mut to_iter: Endpoint, other: &Endpoint, pixels: &[Rgba; 16], offsets: &[i16]) -> [u16; 4] {
	let (mut best, mut index, mut offset) = (bc1_total_dist(&bc1_build_table(&to_iter, other), pixels), 0, 0);
	for i in 0..3 {
		for o in offsets.into_iter() {
			let mut x = to_iter;
			x[i] += *o;
			let table = bc1_build_table(&x, &other);
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
}*/

fn bc1_encode_block(pixels: &[Rgba; 16]) -> u64 {
	let (min, max) = bc1_minmax(&pixels);

	/*for _ in 0..quality {
		min = iter_endpoint(min, &max, &pixels, &[2, 4, 8]);
		max = iter_endpoint(max, &min, &pixels, &[-2, -4, -8]);
	}*/

	let table = bc1_build_table(&min, &max);

	let min = bc1_encode_endpoint(&min) as u64;
	let max = bc1_encode_endpoint(&max) as u64;

	let encoded = bc1_encode_pixel(&table, &pixels) as u64;

	(encoded << 32) | (min << 16) | max
}

pub fn bc1(image: &Vec<u8>, size: (usize, usize), _: u8) -> Result<Vec<u8>, ()> {
	if !is_blk(size) {
		return Err(());
	}

    let mut out = Vec::with_capacity(image.len() / 8);

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

   			let mut encoded = bc1_encode_block(&pixels);

   			for _ in 0..8 {
   				out.push((encoded & 0xFF) as u8);
   				encoded >>= 8;
   			}
		}
	}
	Ok(out)
}
