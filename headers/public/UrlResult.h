/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <string>


namespace BPrivate {

namespace Network {

class BUrlResult {
public:
							BUrlResult();
							~BUrlResult();

			void			SetContentType(const std::string& contentType);
			void			SetLength(off_t length);

			std::string		ContentType() const;
			off_t			Length() const;

private:
			std::string		fContentType;
			off_t			fLength;
};

} // namespace Network

} // namespace BPrivate

#endif // _B_URL_RESULT_H_
