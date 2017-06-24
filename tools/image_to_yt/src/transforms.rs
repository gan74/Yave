use rayon::prelude::*;

use image_data::*;


fn is_sq_power_2(size: (usize, usize)) -> bool {
	size.0 == size.1 && size.0.count_ones() == 1
}



pub fn apply_gamma(image: &ImageData, gamma: f32) -> Option<ImageData> {
	let mut out = Vec::with_capacity(image.data.len());

	image.data.par_iter().map(|p| ((*p as f32 / 255.0).powf(gamma) * 255.0) as u8).collect_into(&mut out);

	Some(ImageData {
		size: image.size,
		data: out
	})
}

pub fn mipmap(image: &ImageData) -> Option<ImageData> {
	if !is_sq_power_2(image.size) || 
	   image.size.0 < 2 || image.size.1 < 2 || 
	   image.data.len() != (image.size.0 * image.size.1 * 4) {

	   return None;
	}

	let mut out = Vec::with_capacity(image.data.len() / 4);
	for x in 0..(image.size.0 / 2) {
		let x2 = 2 * x;
		for y in 0..(image.size.1 / 2) { 
			let y2 = 2 * y;
			for c in 0..4 {
				let (a, b, c, d) = (image.data[(x2 * image.size.0 + y2) * 4 + c], 
									image.data[((x2 + 1) * image.size.0 + y2) * 4 + c],
									image.data[(x2 * image.size.0 + y2 + 1) * 4 + c],
									image.data[((x2 + 1) * image.size.0 + y2 + 1) * 4 + c]);
				let sum = a as u32  + b as u32 + c as u32 + d as u32;
				out.push((sum / 4) as u8);
			}
		}
	}

	Some(ImageData {
		size: (image.size.0 / 2, image.size.1 / 2),
		data: out
	})
}