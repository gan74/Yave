extern crate image;

use std::fs::{File};
use std::path::{Path};
use std::io::{BufWriter,Write, Result};
use std::mem;
use std::slice;

use image::*;


fn main() {
    let mut args = std::env::args();
    args.next();

    let file_name = args.next().expect("Expected a file as input.");
    let image = image::open(&Path::new(&file_name)).expect("Unable to open image file.");

    let ref mut writer = BufWriter::new(File::create(file_name + ".rgba").expect("Unable to create output file."));

    write_image(writer, &image).expect("Unable to write to output file.");
}



fn write_image<T: Write>(file: &mut T, image: &DynamicImage) -> Result<usize> {
    let rgba = image.flipv().to_rgba();
    let dims = vec![image.dimensions().0 as u32, image.dimensions().1 as u32];

    write_bin(file, &dims)
        .and_then(|_| write_bin(file, &rgba.to_vec()))
}

fn write_bin<E, T: Write>(file: &mut T, v: &Vec<E>) -> Result<usize> {
    let slice_u8: &[u8] = unsafe {
        slice::from_raw_parts(
            v.as_ptr() as *const u8, 
            v.len() * mem::size_of::<E>()
        )
    };
    file.write(slice_u8)
}