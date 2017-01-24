
use mipmaping::*;
use image::*;

pub struct ImageData {
	pub size: (usize, usize),
	pub data: Vec<u8>
}

impl ImageData {
	pub fn from_image(img: &DynamicImage) -> ImageData {
		ImageData {
			size: (img.dimensions().0 as usize, img.dimensions().1 as usize),
			data: img.flipv().to_rgba().to_vec()
		}
	}

	pub fn mipmap(&self) -> Option<ImageData> {
		mipmap(&self.data, self.size).map(|m| ImageData{ size: m.1, data: m.0})
	}
}
