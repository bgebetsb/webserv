/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Post.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbonengl <mbonengl@student.42vienna.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/20 19:25:39 by mbonengl          #+#    #+#             */
/*   Updated: 2025/05/20 19:27:38 by mbonengl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstddef>
#include <string>
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "../utils/Utils.hpp"
#include "Configs/Configs.hpp"
#include "Option.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "RequestStatus.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestMethods.hpp"
#include "responses/FileResponse.hpp"
#include "responses/Response.hpp"

//
//
//
//
//
//
//
//
//
