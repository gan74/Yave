extern crate image;
extern crate rayon;

use std::fs::{File};
use std::path::{Path};
use std::time::{SystemTime};
use std::io::{BufWriter, Write, SeekFrom, Seek, Result, Error, ErrorKind};
use std::mem;
use std::env;
use std::slice;

mod image_data;
mod image_format;
mod transforms;
mod bc1;
mod bc5;

use image_data::*;
use image_format::*;

fn main() {
    let mut format: Box<ImageFormat> = Box::new(Rgba8Format::new());
    let mut quality = 0u8;
    let mut gamma = 1.0;
    let mut unused_arguments = false;

    for arg in env::args().skip(1) {
        if arg.starts_with("--quality=") {
            unused_arguments = true;
            quality = arg[10..].parse::<u32>().expect("Invalid quality") as u8;

        } else if arg.starts_with("--gamma=") {
            unused_arguments = true;
            gamma = arg[8..].parse::<f32>().expect("Invalid gamma");
            assert!(gamma > 0.0, "gamma must be strictly positive");

        } else if arg.starts_with("-") {
            unused_arguments = true;
            format = match arg.as_ref() {
                "--bc1" => Box::new(Bc1Format::new()),
                "--bc5" => Box::new(Bc5Format::new()),
                "--rgba" => Box::new(Rgba8Format::new()),
                _ => panic!("Unknown argument.")
            }

        } else {
            unused_arguments = false;
            let timer = start();

            let mut image = load_image(&arg);

            if gamma != 1.0 {
                image = transforms::apply_gamma(&image, gamma).unwrap();
            }

            write_image_to_file(arg, image, format.as_ref(), quality).unwrap();

            stop(timer);
        }
    } 
    if(unused_arguments) {
        println!("Warning: unused aguments");
    }
}




fn load_image(file_name: &String) -> ImageData {
    print!("{}: ", file_name);
    ImageData::open(&Path::new(file_name)).expect("Unable to open image file.")
}

fn write_image_to_file(file_name: String, mut image: ImageData, format: &ImageFormat, quality: u8) -> Result<usize> {
    let ref mut writer = BufWriter::new(File::create(file_name + ".yt").expect("Unable to create output file."));
    write_image(writer, image, format, quality)
}

fn write_image<W: Write + Seek>(file: &mut BufWriter<W>, mut image: ImageData, format: &ImageFormat, quality: u8) -> Result<usize> {
    let image_type: u32 = 2;
    let version: u32 = 2;
    let size = (image.size.0 as u32, image.size.1 as u32);

    file.write(b"yave")
        .and_then(|_| write_bin(file, &vec![image_type, version, size.0, size.1, 0, format.id()]))?;

    match format.encode(&image, quality) {
        Ok(d) => write_bin(file, &d)?,
        Err(_) => return Err(Error::new(ErrorKind::Other, "Unable to encode image."))
    };

    let mut mips = 1u32;
    while let Some(next) = transforms::mipmap(&image) {
        if let Ok(enc) = format.encode(&next, quality) {
            mips += 1;
            image = next;
            write_bin(file, &enc)?;
        } else {
            break;
        }
    }

    println!("{} mipmaps exported.", mips);
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