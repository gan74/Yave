extern crate image;

use std::fs::{File};
use std::path::{Path};
use std::time::{SystemTime};
use std::io::{BufWriter, Write, SeekFrom, Seek, Result, Error, ErrorKind};
use std::mem;
use std::slice;

mod image_data;
mod image_format;
mod mipmaping;
mod bc;

use image_data::*;
use image_format::{ImageFormat, Bc1Format};

fn main() {
    let mut args = std::env::args();
    args.next();

    let file_name = args.next().expect("Expected a file as input.");
    let image = ImageData::from_image(&image::open(&Path::new(&file_name)).expect("Unable to open image file."));

    let ref mut writer = BufWriter::new(File::create(file_name + ".yt").expect("Unable to create output file."));

    write_image(writer, image, &Bc1Format::new()).expect("Unable to write to output file.");
}

    
fn write_image<W: Write + Seek>(file: &mut BufWriter<W>, mut image: ImageData, format: &ImageFormat) -> Result<usize> {
    let image_type: u32 = 2;
    let version: u32 = 1;
    let size = (image.size.0 as u32, image.size.1 as u32);

    file.write(b"yave")
        .and_then(|_| write_bin(file, &vec![image_type, version, size.0, size.1, 0, format.id()]))?;

    let timer = start();
    match format.encode(&image) {
        Ok(d) => write_bin(file, &d)?,
        Err(_) => return Err(Error::new(ErrorKind::Other, "Unable to encode image."))
    };
    stop(timer);

    let mut mips = 1u32;
    while let Some(next) = image.mipmap() {
        if let Ok(enc) = format.encode(&next) {
            mips += 1;
            image = next;
            write_bin(file, &enc)?;
        } else {
            break;
        }
    }
    println!("{} mipmaps exported", mips);
    file.seek(SeekFrom::Start(20))
        .and_then(|_| write_bin(file, &vec![mips]))
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

fn start() -> SystemTime {
    SystemTime::now()
}

fn stop(t: SystemTime) {
    let d = t.elapsed().unwrap();
    let msec = d.subsec_nanos() as u64 / 1000_000 + d.as_secs() * 1000;
    println!("{}ms", msec);
}