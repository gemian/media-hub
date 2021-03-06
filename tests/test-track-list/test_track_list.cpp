/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include "test_track_list.h"
#include "../../src/core/media/util/timeout.h"

#include <core/media/service.h>
#include <core/media/track_list.h>

#include <cassert>
#include <future>
#include <iostream>
#include <thread>

namespace media = core::ubuntu::media;
using namespace std;

media::TestTrackList::TestTrackList()
{
    try {
        m_hubService = media::Service::Client::instance();
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to connect to media-hub service: " << e.what() << endl;
    }
}

media::TestTrackList::~TestTrackList()
{
}

std::string media::TestTrackList::create_new_player_session()
{
    try {
        m_hubPlayerSession = m_hubService->create_session(media::Player::Client::default_configuration());
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to start a new media-hub player session: " << e.what() << endl;
    }

    try {
        m_hubTrackList = m_hubPlayerSession->track_list();
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to retrieve the current player's TrackList: " << e.what() << endl;
    }

    std::string uuid;
    try {
        uuid.assign(m_hubPlayerSession->uuid());
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to retrieve the current player's uuid: " << e.what() << endl;
    }

    cout << "Connected to session " << uuid << endl;
    return uuid;
}

void media::TestTrackList::detach_player_session(const std::string &uuid)
{
    try {
        m_hubService->detach_session(uuid, media::Player::Client::default_configuration());
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to detach the media-hub player session: " << e.what() << endl;
    }

    cout << "Detached session " << uuid << endl;
}

void media::TestTrackList::reattach_player_session(const std::string &uuid)
{
    try {
        m_hubPlayerSession = m_hubService->reattach_session(uuid, media::Player::Client::default_configuration());
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to reattach the media-hub player session: " << e.what() << endl;
    }

    try {
        m_hubTrackList = m_hubPlayerSession->track_list();
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to retrieve the current player's TrackList: " << e.what() << endl;
    }

    cout << "Re-connected to session " << uuid << endl;
}

void media::TestTrackList::destroy_player_session(const std::string &uuid)
{
    m_hubService->destroy_session(uuid, media::Player::Client::default_configuration());
    m_hubPlayerSession.reset();
}

void media::TestTrackList::add_track(const string &uri, bool make_current)
{
    assert (m_hubTrackList.get() != nullptr);

    cout << "Adding " << uri << " to the TrackList for playback." << endl;

    try {
        bool can_edit_tracks = m_hubTrackList->can_edit_tracks();
        cout << "can_edit_tracks: " << (can_edit_tracks ? "yes" : "no") << std::endl;
        if (can_edit_tracks)
            m_hubTrackList->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), make_current);
        else
            cerr << "Can't add track to TrackList since can_edit_tracks is false" << endl;
    }
    catch (std::runtime_error &e) {
        cerr << "ERROR: Failed to add track " << uri << " to tracklist: " << e.what() << endl;
    }
}

#include <unistd.h>
void media::TestTrackList::test_basic_playback(const std::string &uri1, const std::string &uri2)
{
    cout << "--> Running test: test_basic_playback" << std::endl;

    const std::string uuid = create_new_player_session();

    core::Connection c = m_hubTrackList->on_track_added().connect([](const media::Track::Id &new_id)
    {
        cout << "Added track to TrackList with Id: " << new_id << endl;
    });

    m_hubPlayerSession->open_uri(uri1);
    if (!uri2.empty())
        add_track(uri2);

    //cout << "Waiting for track to be added to TrackList" << endl;
    //media::Track::Id id = wait_for_on_track_added();

    m_hubPlayerSession->play();
    m_hubPlayerSession->loop_status() = media::Player::LoopStatus::none;

    if (m_hubPlayerSession->playback_status() == media::Player::PlaybackStatus::playing)
    {
        cout << "Waiting for first track to finish playing..." << endl;
        wait_for_about_to_finish();
        cout << "Basic playback was successful" << endl;
    }

    c.disconnect();

    destroy_player_session(uuid);
}

void media::TestTrackList::test_tracklist_resume(const std::string &uri1, const std::string &uri2, const std::string &uuid)
{
    cout << "--> Running test: test_tracklist_resume" << std::endl;

    add_track(uri1);
    add_track(uri2);

    const size_t initial_size = m_hubTrackList->tracks().get().size();
    cout << "Tracklist size: " << initial_size << endl;
    detach_player_session(uuid);
    reattach_player_session(uuid);
    cout << "Tracklist size: " << m_hubTrackList->tracks().get().size() << endl;

    m_hubPlayerSession->play();

    if (initial_size != m_hubTrackList->tracks().get().size())
        cout << "Tracklist sizes are different, error in resuming" << endl;
}

void media::TestTrackList::test_ensure_tracklist_is_not_empty(const std::string &uri1, const std::string &uri2)
{
    cout << "--> Running test: test_ensure_tracklist_is_not_empty" << std::endl;

    const std::string uuid = create_new_player_session();

    add_track(uri1);
    if (!uri2.empty())
        add_track(uri2);

    if (m_hubTrackList->tracks()->size() == 1 or m_hubTrackList->tracks()->size() == 2)
        cout << "TrackList is not empty, test success" << endl;
    else
        cout << "TrackList is empty, test failure" << endl;

    destroy_player_session(uuid);
}

void media::TestTrackList::test_has_next_track(const std::string &uri1, const std::string &uri2)
{
    cout << "--> Running test: test_has_next_track" << std::endl;

    const std::string uuid = create_new_player_session();

    add_track(uri1);
    add_track(uri2);

    m_hubPlayerSession->play();
    m_hubPlayerSession->loop_status() = media::Player::LoopStatus::none;

    if (m_hubPlayerSession->playback_status() == media::Player::PlaybackStatus::playing)
    {
        cout << "Waiting for first track to finish playing..." << endl;
        wait_for_about_to_finish();
        cout << "Waiting for second track to finish playing..." << endl;
        wait_for_about_to_finish();
        cout << "Both tracks played successfully" << endl;
    }
    else
        cerr << "Playback did not start successfully" << endl;

    destroy_player_session(uuid);
}

void media::TestTrackList::test_shuffle(const std::string &uri1, const std::string &uri2, const std::string &uri3)
{
    cout << "--> Running test: test_shuffle" << std::endl;

    const std::string uuid = create_new_player_session();

    add_track(uri1);
    add_track(uri2);
    add_track(uri3);
    add_track(uri3);
    add_track(uri2);
    add_track(uri1);
    add_track(uri1);
    add_track(uri2);

    m_hubPlayerSession->play();
    m_hubPlayerSession->loop_status() = media::Player::LoopStatus::playlist;
    m_hubPlayerSession->shuffle() = true;

    if (m_hubPlayerSession->playback_status() == media::Player::PlaybackStatus::playing)
    {
        cout << "Waiting for first track to finish playing..." << endl;
        wait_for_about_to_finish();

        cout << "Turning off shuffle mode" << endl;
        m_hubPlayerSession->shuffle() = false;

        cout << "Waiting for second track to finish playing..." << endl;
        wait_for_about_to_finish();

        const media::Track::Id id{m_hubTrackList->tracks().get()[3]};
        cout << "Going straight to the Track with Id " << id << std::endl;
        m_hubTrackList->go_to(id);

        cout << "Waiting for third track to finish playing..." << endl;
        wait_for_about_to_finish();
    }
    else
        cerr << "Playback did not start successfully" << endl;

    destroy_player_session(uuid);
}

void media::TestTrackList::test_remove_track(const std::string &uri1, const std::string &uri2, const std::string &uri3)
{
    cout << "--> Running test: test_remove_track" << std::endl;

    const std::string uuid = create_new_player_session();

    core::Connection c = m_hubTrackList->on_track_removed().connect([](const media::Track::Id &new_id)
    {
        cout << "Removed track from TrackList with Id: " << new_id << endl;
    });

    add_track(uri1);
    add_track(uri2);
    add_track(uri3);

    m_hubPlayerSession->play();

    if (m_hubPlayerSession->playback_status() == media::Player::PlaybackStatus::playing)
    {
        cout << "Waiting for first track to finish playing..." << endl;
        wait_for_about_to_finish();

        const media::Track::Id id{m_hubTrackList->tracks().get()[1]};
        cout << "Removing Track with Id of '" << id << "'" << std::endl;
        m_hubTrackList->remove_track(id);

        cout << "Waiting for track after removed track to finish playing..." << endl;
        wait_for_about_to_finish();
    }
    else
        cerr << "Playback did not start successfully" << endl;

    destroy_player_session(uuid);
}

template<class T>
bool media::TestTrackList::verify_signal_is_emitted(const core::Signal<T> &signal, const std::chrono::milliseconds &timeout)
{
    bool signal_emitted = false;
#if 0
            static const std::chrono::milliseconds timeout{1000};
            media::timeout(timeout.count(), true, [wp]()
            {
                if (auto sp = wp.lock())
                    sp->on_client_died();
            });
    signal.connect([signal_emitted]()
    {
    });
#endif

    return false;
}

void media::TestTrackList::wait_for_about_to_finish()
{
    bool received_about_to_finish = false;
    core::Connection c = m_hubPlayerSession->about_to_finish().connect([&received_about_to_finish]()
    {
        cout << "AboutToFinish signaled" << endl;
        received_about_to_finish = true;
    });
    while (!received_about_to_finish)
        std::this_thread::yield();

    c.disconnect();
}

void media::TestTrackList::wait_for_end_of_stream()
{
    bool reached_end_of_first_track = false;
    core::Connection c = m_hubPlayerSession->end_of_stream().connect([&reached_end_of_first_track]()
    {
        cout << "EndOfStream signaled" << endl;
        reached_end_of_first_track = true;
    });
    while (!reached_end_of_first_track)
        std::this_thread::yield();

    c.disconnect();
}

void media::TestTrackList::wait_for_playback_status_changed(core::ubuntu::media::Player::PlaybackStatus status)
{
    bool received_playback_status_changed = false;
    core::Connection c = m_hubPlayerSession->playback_status().changed().connect([&received_playback_status_changed, &status](media::Player::PlaybackStatus new_status)
    {
        cout << "PlaybackStatusChanged signaled" << endl;
        if (new_status == status)
            received_playback_status_changed = true;
    });
    while (!received_playback_status_changed)
        std::this_thread::yield();

    c.disconnect();
}

core::ubuntu::media::Track::Id media::TestTrackList::wait_for_on_track_added()
{
    media::Track::Id id;
    bool received_on_track_added = false;
    core::Connection c = m_hubTrackList->on_track_added().connect([&received_on_track_added, &id](const media::Track::Id &new_id)
    {
        cout << "OnTrackAdded signaled" << endl;
        id = new_id;
        received_on_track_added = true;
    });
    while (!received_on_track_added)
        std::this_thread::yield();

    c.disconnect();

    return id;
}

int main (int argc, char **argv)
{
    shared_ptr<media::TestTrackList> tracklist = make_shared<media::TestTrackList>();

    if (argc == 2)
    {
        tracklist->test_basic_playback(argv[1]);
        tracklist->test_ensure_tracklist_is_not_empty(argv[1]);
    }
    else if (argc == 3)
    {
        tracklist->test_basic_playback(argv[1], argv[2]);
        tracklist->test_ensure_tracklist_is_not_empty(argv[1], argv[2]);
        tracklist->test_has_next_track(argv[1], argv[2]);
    }
    else if (argc == 4)
    {
        tracklist->test_basic_playback(argv[1]);
        tracklist->test_ensure_tracklist_is_not_empty(argv[1], argv[2]);
        tracklist->test_has_next_track(argv[1], argv[2]);
        tracklist->test_shuffle(argv[1], argv[2], argv[3]);
        tracklist->test_remove_track(argv[1], argv[2], argv[3]);
    }
    else if (argc == 5)
    {
        std::string uuid = tracklist->create_new_player_session();

        tracklist->test_tracklist_resume(argv[1], argv[2], uuid);

        tracklist->destroy_player_session(uuid);
    }
    else
    {
        cout << "Can't test TrackList, no track(s) specified." << endl;
        cout << argv[0] << " <track_uri_1> [<track_uri_2>] [<track_uri_3>]" << endl;
    }

    return 0;
}
