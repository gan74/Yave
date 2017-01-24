
use bc::*;
use image_data::*;


pub trait ImageFormat {
	fn id(&self) -> u32;
	fn encode(&self, &ImageData) -> Result<Vec<u8>, ()>;
}




pub struct Rgba8Format {
}

impl Rgba8Format {
	pub fn new() -> Rgba8Format {
		Rgba8Format{}
	}
}

impl ImageFormat for Rgba8Format {
	fn id(&self) -> u32 {
		37
	}

	fn encode(&self, image: &ImageData) -> Result<Vec<u8>, ()> {
		Ok(image.data.clone())
	}
}




pub struct Bc1Format {
}

impl Bc1Format {
	pub fn new() -> Bc1Format {
		Bc1Format{}
	}
}

impl ImageFormat for Bc1Format {
	fn id(&self) -> u32 {
		133
	}

	fn encode(&self, image: &ImageData) -> Result<Vec<u8>, ()> {
		bc1(&image.data, image.size)
	}
}