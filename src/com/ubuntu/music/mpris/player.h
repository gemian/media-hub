/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MPRIS_PLAYER_H_
#define MPRIS_PLAYER_H_

#include <com/ubuntu/music/track.h>

#include "macros.h"

#include <org/freedesktop/dbus/types/any.h>
#include <org/freedesktop/dbus/types/object_path.h>
#include <org/freedesktop/dbus/types/variant.h>

#include <boost/utility/identity_type.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = org::freedesktop::dbus;

namespace mpris
{
struct Player
{
    static const std::string& name()
    {
        static const std::string s{"com.ubuntu.music.Service.Player"};
        return s;
    }

    METHOD(Next, Player, std::chrono::seconds(1))
    METHOD(Previous, Player, std::chrono::seconds(1))
    METHOD(Pause, Player, std::chrono::seconds(1))
    METHOD(PlayPause, Player, std::chrono::seconds(1))
    METHOD(Stop, Player, std::chrono::seconds(1))
    METHOD(Play, Player, std::chrono::seconds(1))
    METHOD(Seek, Player, std::chrono::seconds(1))
    METHOD(SetPosition, Player, std::chrono::seconds(1))
    METHOD(OpenUri, Player, std::chrono::seconds(1))

    struct Signals
    {
        SIGNAL(Seeked, Player, uint64_t)
    };

    struct Properties
    {
        READABLE_PROPERTY(PlaybackStatus, Player, std::string)
        WRITABLE_PROPERTY(LoopStatus, Player, std::string)
        WRITABLE_PROPERTY(PlaybackRate, Player, double)
        WRITABLE_PROPERTY(Rate, Player, double)
        WRITABLE_PROPERTY(Shuffle, Player, bool)
        READABLE_PROPERTY(MetaData, Player, BOOST_IDENTITY_TYPE((std::map<std::string, std::string>)))
        WRITABLE_PROPERTY(Volume, Player, double)
        READABLE_PROPERTY(Position, Player, uint64_t)
        READABLE_PROPERTY(MinimumRate, Player, double)
        READABLE_PROPERTY(MaximumRate, Player, double)
        READABLE_PROPERTY(CanGoNext, Player, bool)
        READABLE_PROPERTY(CanGoPrevious, Player, bool)
        READABLE_PROPERTY(CanPlay, Player, bool)
        READABLE_PROPERTY(CanPause, Player, bool)
        READABLE_PROPERTY(CanSeek, Player, bool)
        READABLE_PROPERTY(CanControl, Player, bool)
    };
};
}

#endif // MPRIS_PLAYER_H_