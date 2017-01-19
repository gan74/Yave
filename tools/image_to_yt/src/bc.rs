

fn is_blk(size: (u32, u32)) -> bool {
	return size.0 % 4 == 0 && size.1 % 4 == 0;
}

pub fn bc1(image: &Vec<u8>, size: (u32, u32)) -> Option<Vec<u8>> {
	if !is_blk(size) {
		return None;
	}

    let mut out = Vec::new();
    /*let mip1 = mipmap(image, size).unwrap();
    let mip2 = mipmap(&mip1.0, mip1.1).unwrap();

    for i in 0..((mip2.1).0 * (mip2.1).1) {
    	let i = i as usize;
    	let rgb = (mip2.0[i * 4] as u16, mip2.0[i * 4 + 1] as u16, mip2.0[i * 4 + 2] as u16);

		let pixel: u16 = ((rgb.0 >> 3) << 11) |
						 ((rgb.1 >> 2) << 5) |
						 (rgb.2 >> 3);

		out.push((pixel & 0xFF) as u8);
		out.push((pixel >> 8) as u8);

		out.push((pixel & 0xFF) as u8);
		out.push((pixel >> 8) as u8);

		out.push(0x22 as u8);
		out.push(0x22 as u8);
		out.push(0x22 as u8);
		out.push(0x22 as u8);
    }*/

   	for x in 0..(size.0 / 4) {
   		for y in 0..(size.1 / 4) {
   			let index = (y * 4 + x * 4 * size.0) as usize * 4 ;

			let rgb = (image[index] as u16, image[index + 1] as u16, image[index + 2] as u16);

			let pixel: u16 = ((rgb.0 >> 3) << 11) |
							 ((rgb.1 >> 2) << 5) |
							 (rgb.2 >> 3);

			out.push((pixel & 0xFF) as u8);
			out.push((pixel >> 8) as u8);

			out.push((pixel & 0xFF) as u8);
			out.push((pixel >> 8) as u8);

			out.push(0x22 as u8);
			out.push(0x22 as u8);
			out.push(0x22 as u8);
			out.push(0x22 as u8);
		}
	}

	Some(out)
}
