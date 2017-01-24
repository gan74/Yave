fn is_sq_power_2(size: (usize, usize)) -> bool {
    size.0 == size.1 && size.0.count_ones() == 1
}

pub fn mipmap(rgba: &Vec<u8>, size: (usize, usize)) -> Option<(Vec<u8>, (usize, usize))> {
    if !is_sq_power_2(size) || 
       size.0 < 2 || size.1 < 2 || 
       rgba.len() != (size.0 * size.1 * 4) {

        return None;
    }
    let mut out = Vec::new();
    for x in 0..(size.0 / 2) {
        let x2 = 2 * x;
        for y in 0..(size.1 / 2) { 
            let y2 = 2 * y;
            for c in 0..4 {
                let (a, b, c, d) = (rgba[(x2 * size.0 + y2) * 4 + c], 
                                    rgba[((x2 + 1) * size.0 + y2) * 4 + c],
                                    rgba[(x2 * size.0 + y2 + 1) * 4 + c],
                                    rgba[((x2 + 1) * size.0 + y2 + 1) * 4 + c]);
                let sum = a as u32  + b as u32 + c as u32 + d as u32;
                out.push((sum / 4) as u8);
            }
        }
    }

    Some((out, (size.0 / 2, size.1 / 2)))
}