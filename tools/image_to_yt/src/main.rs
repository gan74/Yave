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
	let mut inputs = Vec::new();

	for arg in env::args().skip(1) {
		if arg.starts_with("--quality=") {
			quality = arg[10..].parse::<u32>().expect("Invalid quality") as u8;

		} else if arg.starts_with("--gamma=") {
			gamma = arg[8..].parse::<f32>().expect("Invalid gamma");
			assert!(gamma > 0.0, "gamma must be strictly positive");

		} else if arg.starts_with("-") {
			format = match arg.as_ref() {
				"--bc1" => Box::new(Bc1Format::new()),
				"--bc5" => Box::new(Bc5Format::new()),
				"--rgba" => Box::new(Rgba8Format::new()),
				_ => panic!("Unknown argument.")
			}

		} else {
			inputs.push(arg);

		}
	} 
	

	let timer = start();
	let images = inputs.iter().map(|path| {
			let mut image = ImageData::open(Path::new(path)).expect("Unable to open image file.");
			if gamma != 1.0 {
				image = transforms::apply_gamma(&image, gamma).unwrap();
			}
			image
		}).collect();

		
	write_images_to_file(Path::new(inputs.first().expect("No input specified")), images, format.as_ref(), quality).unwrap();

	stop(timer);
}


fn write_images_to_file(path: &Path, 
						images: Vec<ImageData>, 
						format: &ImageFormat, 
						quality: u8) -> Result<usize> {

	let ref mut writer = BufWriter::new(File::create(path.with_extension("yt")).expect("Unable to create output file."));
	write_images(writer, images, format, quality)
}


fn write_images<W: Write + Seek>(file: &mut BufWriter<W>, 
								 images: Vec<ImageData>, 
								 format: &ImageFormat, 
								 quality: u8) -> Result<usize> {
	let image_type: u32 = 2;
	let version: u32 = 3;
	let size = (images[0].size.0 as u32, images[0].size.1 as u32);
	let layers = images.len() as u32;

	for image in &images {
		if image.size.0 as u32 != size.0 || image.size.1 as u32 != size.1 {
			return Err(Error::new(ErrorKind::Other, "Layers have mismatched sizes."))
		}
	}

	file.write(b"yave")
		.and_then(|_| write_bin(file, &[image_type, version, size.0, size.1, layers, 0, format.id()]))?;

	let mut mipmaps = 0;
	for layer in images {
		let mut image = layer;
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
		mipmaps = mips;
	}
	println!("{} mipmaps exported", mipmaps);
	
	file.seek(SeekFrom::Start(24))
		.and_then(|_| write_bin(file, &[mipmaps]))
}




fn write_bin<E, T: Write>(file: &mut T, v: &[E]) -> Result<usize> {
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