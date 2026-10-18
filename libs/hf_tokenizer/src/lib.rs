use std::ffi::CStr;
use std::os::raw::c_char;
use std::ptr;

use tokenizers::Tokenizer;

pub struct Handle {
    tokenizer: Tokenizer,
}

unsafe fn cstr<'a>(value: *const c_char) -> Option<&'a str> {
    if value.is_null() {
        return None;
    }

    CStr::from_ptr(value).to_str().ok()
}

#[no_mangle]
pub extern "C" fn hf_tokenizer_load(filename: *const c_char) -> *mut Handle {
    let Some(filename) = (unsafe { cstr(filename) }) else {
        return ptr::null_mut();
    };

    let Ok(tokenizer) = Tokenizer::from_file(filename) else {
        return ptr::null_mut();
    };

    Box::into_raw(Box::new(Handle { tokenizer }))
}

#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_encode(
    handle: *mut Handle,
    input: *const c_char,
    output: *mut i32,
    capacity: usize,
) -> i64 {
    if handle.is_null() || output.is_null() {
        return -1;
    }

    let Some(input) = cstr(input) else {
        return -1;
    };

    let Ok(encoded) = (*handle).tokenizer.encode(input, false) else {
        return -1;
    };

    let ids = encoded.get_ids();
    if ids.len() > capacity {
        return -(ids.len() as i64);
    }

    let output = std::slice::from_raw_parts_mut(output, ids.len());
    for (dst, src) in output.iter_mut().zip(ids) {
        *dst = *src as i32;
    }

    ids.len() as i64
}

#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_free(handle: *mut Handle) {
    if !handle.is_null() {
        drop(Box::from_raw(handle));
    }
}
