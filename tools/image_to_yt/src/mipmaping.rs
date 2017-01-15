
use std::cmp::*;

fn is_sq_power_2(size: (u32, u32)) -> bool {
    size.0 == size.1 && size.0.count_ones() == 1
}

/*fn mip_size(lvl: usize, size: (u32, u32)) -> (u32, u32) {
    let factor = 1 << lvl;
    (max(1, size.0 / factor), max(1, size.1 / factor))
}*/

pub fn mip_levels(size: (u32, u32)) -> usize {
    if !is_sq_power_2(size) {
        0
    } else {
        let m = max(size.0, size.1) as f64;
        m.log2() as usize
    }
}

pub fn mipmap(image: &Vec<u8>, size: (u32, u32)) -> Option<(Vec<u8>, (u32, u32))> {
    if !is_sq_power_2(size) || 
       size.0 < 2 || size.1 < 2 || 
       image.len() != (size.0 * size.1 * 4) as usize {

        return None;
    }
    let mut out = Vec::new();
    {
        let size = (size.0 as usize, size.1 as usize);
        for x in 0..(size.0 / 2) {
            let x2 = 2 * x;
            for y in 0..(size.1 / 2) { 
                let y2 = 2 * y;
                for c in 0..4 {
                    let (a, b, c, d) = (image[(x2 * size.0 + y2) * 4 + c], 
                                        image[((x2 + 1) * size.0 + y2) * 4 + c],
                                        image[(x2 * size.0 + y2 + 1) * 4 + c],
                                        image[((x2 + 1) * size.0 + y2 + 1) * 4 + c]);
                    let sum = a as u32  + b as u32 + c as u32 + d as u32;
                    out.push((sum / 4) as u8);
                }
            }
        }
    }

    Some((out, (size.0 / 2, size.1 / 2)))
}