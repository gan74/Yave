
use image_data::*;
use bc1;
use bc5;


pub trait ImageFormat {
	fn id(&self) -> u32;
	fn encode(&self, &ImageData, u8) -> Result<Vec<u8>, ()>;
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

	fn encode(&self, image: &ImageData, _: u8) -> Result<Vec<u8>, ()> {
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

	fn encode(&self, image: &ImageData, quality: u8) -> Result<Vec<u8>, ()> {
		bc1::encode(image, quality)
	}
}


pub struct Bc5Format {
}

impl Bc5Format {
	pub fn new() -> Bc5Format {
		Bc5Format{}
	}
}

impl ImageFormat for Bc5Format {
	fn id(&self) -> u32 {
		141
	}

	fn encode(&self, image: &ImageData, _: u8) -> Result<Vec<u8>, ()> {
		bc5::encode(image)
	}
}