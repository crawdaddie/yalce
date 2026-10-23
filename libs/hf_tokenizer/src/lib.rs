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

// A null output queries the UTF-8 byte count; no trailing NUL is written.
#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_decode(
    handle: *mut Handle,
    token: i32,
    output: *mut c_char,
    capacity: i32,
) -> i32 {
    hf_tokenizer_decode_ids(handle, &token, 1, output, capacity)
}

// Decode together: UTF-8 characters may span several tokens.
#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_decode_ids(
    handle: *mut Handle,
    input: *const i32,
    count: i32,
    output: *mut c_char,
    capacity: i32,
) -> i32 {
    const DECODE_ERROR: i32 = -1;

    if handle.is_null() || count < 0 || capacity < 0 || (count > 0 && input.is_null()) {
        return DECODE_ERROR;
    }

    let tokenizer = &(*handle).tokenizer;
    let ids = if count == 0 {
        &[]
    } else {
        std::slice::from_raw_parts(input, count as usize)
    };
    if ids
        .iter()
        .any(|&id| id < 0 || tokenizer.id_to_token(id as u32).is_none())
    {
        return DECODE_ERROR;
    }
    let ids: Vec<u32> = ids.iter().map(|&id| id as u32).collect();

    let Ok(text) = tokenizer.decode(&ids, false) else {
        return DECODE_ERROR;
    };
    let Ok(length) = i32::try_from(text.len()) else {
        return DECODE_ERROR;
    };

    if output.is_null() {
        return length;
    }
    if capacity < length {
        return DECODE_ERROR;
    }

    ptr::copy_nonoverlapping(text.as_ptr(), output.cast(), text.len());
    length
}

#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_free(handle: *mut Handle) {
    if !handle.is_null() {
        drop(Box::from_raw(handle));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tokenizers::models::wordlevel::WordLevel;

    fn handle() -> Handle {
        let model = WordLevel::builder()
            .vocab([(" I".to_owned(), 0), (" café".to_owned(), 1)].into())
            .build()
            .unwrap();
        Handle {
            tokenizer: Tokenizer::new(model),
        }
    }

    #[test]
    fn decode_preserves_utf8() {
        let mut handle = handle();
        for (token, expected) in [(0, " I"), (1, " café")] {
            let length = unsafe { hf_tokenizer_decode(&mut handle, token, ptr::null_mut(), 0) };
            assert_eq!(length as usize, expected.len());
            let mut output = vec![0_u8; length as usize];
            let written = unsafe {
                hf_tokenizer_decode(&mut handle, token, output.as_mut_ptr().cast(), length)
            };
            assert_eq!(written, length);
            assert_eq!(output, expected.as_bytes());
        }
    }

    #[test]
    fn decode_checks_capacity() {
        let mut handle = handle();
        let mut output = [42_u8];
        let result = unsafe { hf_tokenizer_decode(&mut handle, 0, output.as_mut_ptr().cast(), 1) };
        assert_eq!(result, -1);
        assert_eq!(output, [42]);
    }

    #[test]
    fn decode_rejects_invalid() {
        let mut handle = handle();
        for token in [-1, 999] {
            assert_eq!(
                unsafe { hf_tokenizer_decode(&mut handle, token, ptr::null_mut(), 0) },
                -1
            );
        }
        assert_eq!(
            unsafe { hf_tokenizer_decode(ptr::null_mut(), 0, ptr::null_mut(), 0) },
            -1
        );
    }

    #[test]
    fn decode_sequence() {
        let mut handle = handle();
        let ids = [0, 1];
        let mut output = [0_u8; 32];
        let length = unsafe {
            hf_tokenizer_decode_ids(
                &mut handle,
                ids.as_ptr(),
                ids.len() as i32,
                output.as_mut_ptr().cast(),
                output.len() as i32,
            )
        };
        assert_eq!(&output[..length as usize], " I  café".as_bytes());
        assert_eq!(
            unsafe { hf_tokenizer_decode_ids(&mut handle, ptr::null(), 0, ptr::null_mut(), 0) },
            0
        );
        assert_eq!(
            unsafe { hf_tokenizer_decode_ids(&mut handle, ptr::null(), 1, ptr::null_mut(), 0) },
            -1
        );
    }
}
