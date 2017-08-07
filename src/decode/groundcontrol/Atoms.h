#pragma once

#include "decode/Config.h"

#include <caf/atom.hpp>

namespace decode {

using StartAtom                           = caf::atom_constant<caf::atom("startact")>;
using RecvDataAtom                        = caf::atom_constant<caf::atom("recvdata")>;
using RecvUserPacketAtom                  = caf::atom_constant<caf::atom("recvupkt")>;
using SendUserPacketAtom                  = caf::atom_constant<caf::atom("sendupkt")>;
using SendDataAtom                        = caf::atom_constant<caf::atom("senddata")>;
using SendFwtPacketAtom                   = caf::atom_constant<caf::atom("sendupkt")>;
using SendCmdPacketAtom                   = caf::atom_constant<caf::atom("sendcpkt")>;
using SetProjectAtom                      = caf::atom_constant<caf::atom("setprojt")>;
using RegisterClientAtom                  = caf::atom_constant<caf::atom("regclien")>;
using SetTmViewAtom                       = caf::atom_constant<caf::atom("settmviw")>;
using UpdateTmViewAtom                    = caf::atom_constant<caf::atom("updtmviw")>;
using PushTmUpdatesAtom                   = caf::atom_constant<caf::atom("pushtmup")>;

using RepeatStreamAtom                    = caf::atom_constant<caf::atom("strmrept")>;
using SetStreamDestAtom                   = caf::atom_constant<caf::atom("strmdest")>;

using FwtHashAtom                         = caf::atom_constant<caf::atom("fwthash")>;
using FwtStartAtom                        = caf::atom_constant<caf::atom("fwtstart")>;
using FwtCheckAtom                        = caf::atom_constant<caf::atom("fwtcheck")>;

using FirmwareDownloadStartedEventAtom    = caf::atom_constant<caf::atom("feventbgn")>;
using FirmwareDownloadFinishedEventAtom   = caf::atom_constant<caf::atom("feventfin")>;
using FirmwareSizeRecievedEventAtom       = caf::atom_constant<caf::atom("feventsiz")>;
using FirmwareStartCmdSentEventAtom       = caf::atom_constant<caf::atom("feventscs")>;
using FirmwareStartCmdPassedEventAtom     = caf::atom_constant<caf::atom("feventscp")>;
using FirmwareHashDownloadedEventAtom     = caf::atom_constant<caf::atom("feventhok")>;
using FirmwareErrorEventAtom              = caf::atom_constant<caf::atom("feventerr")>;
using FirmwareProgressEventAtom           = caf::atom_constant<caf::atom("feventprg")>;

}
