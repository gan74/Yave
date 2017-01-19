extern crate image;

use std::fs::{File};
use std::path::{Path};
use std::io::{BufWriter,Write, Result};
use std::mem;
use std::slice;

use image::*;

mod mipmaping;

use mipmaping::*;

fn main() {
    let mut args = std::env::args();
    args.next();

    let file_name = args.next().expect("Expected a file as input.");
    let image = image::open(&Path::new(&file_name)).expect("Unable to open image file.");

    let ref mut writer = BufWriter::new(File::create(file_name + ".yt").expect("Unable to create output file."));

    write_image(writer, &image).expect("Unable to write to output file.");
}

fn write_image<T: Write>(file: &mut T, image: &DynamicImage) -> Result<usize> {

    let mut size: (u32, u32) = image.dimensions();
    let mut data = image.flipv().to_rgba().to_vec();
    let mips = mip_levels(size) as u32;

    let image_type: u32 = 2;
    let version: u32 = 1;

    let mut r = file.write(b"yave")
        .and_then(|_| write_bin(file, &vec![image_type, version]))
        .and_then(|_| write_bin(file, &vec![size]))
        .and_then(|_| write_bin(file, &vec![mips + 1]))
        .and_then(|_| write_bin(file, &data));

    for _ in 0..mips {
        let (d, s) = mipmap(&data, size).expect("Unable to compute mipmap.");
        data = d;
        size = s;
        r = r.and_then(|_| write_bin(file, &data));
    }
    r
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