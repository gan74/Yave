
use std::path::{Path};

use mipmaping::*;
use image::{GenericImage};
use image;


pub const BLOCK_SIZE: usize = 4;
pub const BLOCK_PIXELS: usize = BLOCK_SIZE * BLOCK_SIZE;

pub type Rgba = [u8; 4];
pub type Block = [Rgba; BLOCK_PIXELS];


pub struct ImageData {
	pub size: (usize, usize),
	pub data: Vec<u8>
}

impl ImageData {

	pub fn open(path: &Path) -> Result<ImageData, image::ImageError> {
		match image::open(path) {
			Ok(img) => 
				Ok(ImageData {
					size: (img.dimensions().0 as usize, img.dimensions().1 as usize),
					data: img.flipv().to_rgba().to_vec()
				}),
			Err(e) => Err(e)
		}
	}
	
	pub fn pixels(&self) -> Pixels {
		Pixels {
			index: 0,
			data: self
		}
	}
	
	pub fn blocks(&self) -> Blocks {
		Blocks {
			index: 0,
			data: self
		}
	}

	pub fn mipmap(&self) -> Option<ImageData> {
		mipmap(&self.data, self.size).map(|m| ImageData{ size: m.1, data: m.0})
	}
}



pub struct Blocks<'a> {
	index: usize,
	data: &'a ImageData
}

impl<'a> Blocks<'a> {
	fn block_count(&self) -> (usize, usize) {
		(self.data.size.0 / BLOCK_SIZE, self.data.size.1 / BLOCK_SIZE)
	}

	fn xy(&self) -> (usize, usize) {
		(self.index % (self.data.size.0 / BLOCK_SIZE),
		self.index / (self.data.size.0 / BLOCK_SIZE))
	}
	
	fn block(&self) -> Option<Block> {
		let (x, y) = self.xy();
		let count = self.block_count();
		if x >= count.0 || y >= count.1 {
			None 
		} else {
			let mut pixels = [[0u8; 4]; BLOCK_SIZE * BLOCK_SIZE]; // No compile time constants
   			for py in 0..BLOCK_SIZE {
   				for px in 0..BLOCK_SIZE {
   					let x = x * BLOCK_SIZE + px;
   					let y = y * BLOCK_SIZE + py;

   					let pi = (x + y * self.data.size.0) * 4; // in bytes
   					let pix = [self.data.data[pi], self.data.data[pi + 1], self.data.data[pi + 2], self.data.data[pi + 3]];
   					pixels[px + py * BLOCK_SIZE] = pix;
   				}
   			}
			Some(pixels)
		}
	}
}

impl<'a> Iterator for Blocks<'a> {
	type Item = Block;
	
	fn next(&mut self) -> Option<Block> {
		let b = self.block();
		if b.is_some() {
			self.index += 1;
		}
		b
	}
}

pub struct Pixels<'a> {
	index: usize,
	data: &'a ImageData
}

impl<'a> Iterator for Pixels<'a> {
	type Item = Rgba;
	
	fn next(&mut self) -> Option<Rgba> {
		if self.index > (self.data.size.0 * self.data.size.1) {
			None
		} else {
			let pix = [self.data.data[self.index] * 4,
					   self.data.data[self.index] * 4 + 1,
					   self.data.data[self.index] * 4 + 2,
					   self.data.data[self.index] * 4 + 3];
			self.index += 1;
			Some(pix)
		}
	}
	
	fn count(self) -> usize {
		let size = self.data.size.0 * self.data.size.1;
		if self.index > size {
			0
		} else {
			size - self.index
		}
	}
	
	fn nth(&mut self, n: usize) -> Option<Rgba> {
		self.index += n;
		self.next()
	}
}
