extern crate image;

use std::path::{Path};
use std::env;

fn main() {
	for arg in env::args().skip(1) {
		let img = image::open(Path::new(&arg)).expect("Unable to open image file").to_luma_alpha();

		let mut ascii = String::new();

		let width = img.width();
		ascii.reserve((width + width * img.height()) as usize);
		for (x, _, p) in img.enumerate_pixels() {
			ascii.push(
				if p[1] < 128 {
					' ' 
				} else if p[0] > 128{
					'.'
				} else {
					'#'
				});

			if x == (width - 1) {
				ascii.push('\n');
			}
		}
		println!("{}", ascii);
	}
}

