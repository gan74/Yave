 
use std::cmp;
use std::mem;
use image_data::*;
use rayon::prelude::*;


fn build_endpoints(pixels: &Block, component: usize) -> (u8, u8) {
	let mut min = 255u8;
	let mut max = 0u8;
	for pix in pixels {
		min = cmp::min(min, pix[component]);
		max = cmp::max(max, pix[component]);
	}
	(min, max)
}

fn build_table(ends: &(u8, u8)) -> [u8; 8] {
	let coefs = [(7, 0), (0, 7), (6, 1), (5, 2), (4, 3), (3, 4), (2, 5), (1, 6)];
	let mut table = [0u8; 8];
	for i in 0..8 {
		let c = coefs[i];
		table[i] = (((ends.0 as u16) * c.1 + (ends.1 as u16) * c.0) / 7) as u8;
	}
	table
}
 
 fn encode_channel(pixels: &Block, component: usize) -> u64 {
	let (min, max) = build_endpoints(pixels, component);
	let table = build_table(&(min, max));

	let mut mask = 0u64;
	for pix in pixels.into_iter().rev() {
		let c = pix[component];
		let best = (0..8).into_iter().min_by_key(|x| {
				let x = table[*x]; 
				if x < c { c - x } else { x - c }
			}).unwrap() as u64;
		mask = (mask << 3) | (best & 0x07);
	}
	
	(mask << 16) | ((min as u64) << 8) | (max as u64)
 }
 
 pub fn encode(image: &ImageData) -> Result<Vec<u8>, ()> {
	if image.size.0 % BLOCK_SIZE != 0 || image.size.1 % BLOCK_SIZE != 0 {
		return Err(());
	}
	
	let blocks = image.blocks().count();
    let mut out = Vec::with_capacity(blocks);
	for i in 0..(blocks) {
		out.push((i as u64, 0u64));
	}
	
	let _ = out.as_mut_slice().par_iter_mut()
		.map(|x| {
			let index = x.0 as usize;
			let block = image.blocks().nth(index).unwrap();
			let r = encode_channel(&block, 0);
			let g = encode_channel(&block, 1);
			*x = (r, g);
			1
		}).sum();
	
	unsafe {
		let mut out: Vec<u8> = mem::transmute(out);
		out.set_len(blocks * 16);
		Ok(out)
	}
}