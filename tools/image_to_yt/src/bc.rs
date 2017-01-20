
use std::cmp;

fn is_blk(size: (u32, u32)) -> bool {
	return size.0 % 4 == 0 && size.1 % 4 == 0;
}

fn bc1_encode(pix: [u8; 4]) -> u16 {
	((pix[0] as u16 >> 3) << 11) |
	((pix[1] as u16 >> 2) << 5) |
	(pix[2] as u16 >> 3)
}

fn bc1_interp(min: [u8; 4], max: [u8; 4], index: usize) -> [u8; 4] {
	let coefs = match index {
		0x0 => (3, 0),
		0x1 => (0, 3),
		0x2 => (2 ,1),
		_ => (1, 2)
	};
	let mut pix = [0u8; 4];
	for i in 0..4 {
		pix[i] = ((coefs.1 * (min[i] as u16) + 
				   coefs.0 * (max[i] as u16)) / 3) as u8
	}
	pix
}

fn dist(a: &[u8; 4], b: &[u8; 4]) -> u32 {
	let mut sum = 0u32;
	for i in 0..4 {
		let x = if a[i] < b[i] { b[i] - a[i] } else { a[i] - b[i] } as u32;
		sum += x;
	}
	sum
}

pub fn bc1(image: &Vec<u8>, size: (u32, u32)) -> Option<Vec<u8>> {
	if !is_blk(size) {
		return None;
	}

    let mut out = Vec::new();

    let size = (size.0 as usize, size.1 as usize);

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
   					pixels[px + py * 4] = pix;
   				}
   			}

   			let mut min = [255u8; 4];
   			let mut max = [0u8; 4];
   			for p in pixels.into_iter() {
   				for i in 0..4 {
   					min[i] = cmp::min(min[i], p[i]);
   					max[i] = cmp::max(max[i], p[i]);
   				}
   			}

   			let mut interps = [[0u8; 4]; 4];
   			for i in 0..4 {
   				interps[i] = bc1_interp(min, max, i);
   			}

   			// if we want alpha swap min/max (or encode min first)
			let min = bc1_encode(min);
			let max = bc1_encode(max);

			out.push((max & 0xFF) as u8);
			out.push((max >> 8) as u8);

			out.push((min & 0xFF) as u8);
			out.push((min >> 8) as u8);

			let mut mask = 0u32;

			for pix in pixels.into_iter() {
				let best = (0..4).into_iter().min_by_key(|x| dist(&interps[*x], pix)).unwrap() as u32;
				mask = (mask << 2) | (best & 0x03);
			}

			for _ in 0..4 {
				out.push((mask & 0xFF) as u8);
				mask >>= 8;
			}
		}
	}

	Some(out)
}
