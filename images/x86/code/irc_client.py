import os
import sys
import socket
import curses
import curses.wrapper
import select

def win(stdscr):

    curses.cbreak()

    rows, cols = stdscr.getmaxyx()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLACK)

    socket.setdefaulttimeout(120.0)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)

    server.setblocking(1)

    serv = sys.argv[1]
    server.connect((serv, 6667))

    server.setblocking(0)

    mynick = sys.argv[2]
    chan = sys.argv[3]

    server.send("NICK " + mynick + "\r\nUSER " + mynick + " none none : 0 : 0\r\nJOIN " + chan + "\r\n")

    title_win = curses.newwin(1, cols, 0, 0)
    recv_win = curses.newwin(rows - 2, cols, 1, 0)
    send_win = curses.newwin(1, cols, rows - 1, 0)
    msg_win = curses.newwin(rows - 4, cols - 2, 2, 1)

    title_win.bkgd(ord(' '), curses.color_pair(1))
    recv_win.bkgd(ord(' '), curses.color_pair(2))
    send_win.bkgd(ord(' '), curses.color_pair(3))
    msg_win.bkgd(ord(' '), curses.color_pair(3))

    recv_win.border()

    send_win.nodelay(True)

    recv_win.standout()
    recv_win.addstr(0, 1, "  " + mynick + " on " + serv + ", in " + chan + ".  ", curses.color_pair(2))
    recv_win.standend()
    send_win.addstr(0, 0, "> ")

    msg_win.scrollok(True)

    title_win.addstr("Pedigree IRC Client [Python + ncurses]", curses.color_pair(1))
    title_win.refresh()
    recv_win.refresh()
    send_win.refresh()

    running = True

    # variable names are fun.
    def getnick(first_bit):
        if(first_bit[0] == ':'):
            obfuscated_thing = first_bit[1:]
        else:
            obfuscated_thing = first_bit
        nickbit = obfuscated_thing.split("!")
        return nickbit[0]

    def chunks(s, length):
        if(len(s) <= length):
            yield s
        else:
            for i in range(0, len(s), length):
                yield s[i:i+length]

    def writeline(window, curry, line):
        # TODO: Handle oversized lines
        colsPerLine = cols - 6
        lines = list(chunks(line, colsPerLine))
        for l in lines:
            window.addstr(curry, 1, l)
            curry += 1

        # Are we still out of bounds?
        if(curry > (rows - 5)):
            oldY = curry
            curry = rows - 5
            window.scroll(oldY - curry)
        window.refresh()

        return curry

    def handle(data, client, window, curry):

        if(len(data) == 0):
          return curry

        for line in data:
            if(len(line) == 0):
                continue

            line.rstrip()

            s2 = line.split(" ", 3)
            if(len(s2) <= 1):
                continue

            if(s2[0].lower() == "ping"):

                client.send("PONG wikiforall.net\r\n");
                curry = writeline(window, curry, "Received PING")

            elif(s2[1].lower() == "privmsg"):
                nick = getnick(s2[0])

                if(s2[3][0] == ':'):
                    msg = s2[2] + ": " + nick + ": " + s2[3][1:]
                else:
                    msg = s2[2] + ": " + nick + ": " + s2[3]
                curry = writeline(window, curry, msg)

            elif(s2[1].lower() == "part"):

                msg = ("%s parted in %s") % (getnick(s2[0]), s2[2])
                curry = writeline(window, curry, msg)

            elif(s2[1].lower() == "join"):
								if(len(s2[2]) == 0):
										chan = "unknown"
								elif(s2[2][0] == ':'):
										chan = s2[2][1:]
								else:
										chan = s2[2]

								msg = ("%s joined in %s") % (getnick(s2[0]), chan)
								curry = writeline(window, curry, msg)

            elif(s2[1].lower() == "quit"):
                if(s2[3][0] == ':'):
                    quitmsg = s2[3][1:]
                else:
                    quitmsg = s2[3]

                msg = ("%s quit in %s") % (getnick(s2[0]), chan)
                curry = writeline(window, curry, msg)

            elif(s2[1].lower() == "topic"):

                if(s2[3][0] == ':'):
                    topic = s2[3][1:]
                else:
                    topic = s2[3]

                msg = "topic change to '" + topic + "': changed by " + getnick(s2[0]) + " in " + s2[2]
                curry = writeline(window, curry, msg)

            elif(s2[0].lower() == "notice"):

                if(s2[2][0] == ':'):
                    notice = s2[2][1:]
                else:
                    notice = s2[2]

                curry = writeline(window, curry, "NOTICE: " + s2[1] + notice)

            else:
                curry = writeline(window, curry, line)

        return curry

    def get_line(curseswin):
        curses.echo()
        ret = curseswin.getstr(0, 0)
        curses.noecho()
        return ret

    y = 0
    x = 0

    try:
      readbuffer = server.recv(1024)
      splitted = readbuffer.split("\n")
      readbuffer = splitted.pop()
      y = handle(splitted, server, msg_win, y)
    except:
      pass

    data = ""
    send_x = 2
    send_y = 0

    while 1:
				try:
						ch = send_win.getch()
						if(ch != -1):
							if(chr(ch) == '\n'):
							    if(len(data) > 0):
									  if(data[0] == "/"):
										  server.send(data[1:] + "\r\n")

										  if(data[1:5] == "QUIT" or data[1:5] == "quit"):
											  running = False
											  break
									  else:
										  msg = "PRIVMSG " + chan + " :" + data
										  server.send(msg + "\r\n")
										  y = handle([":pedigree!n=pedigree@202.63.42.160.static.rev.aanet.com.au " + msg], server, msg_win, y)
									  send_win.erase()
									  send_win.addstr(0, 0, "> ")
									  data = ""
							elif(ch == 0x7F or ch == 0x08):
								send_win.addstr(send_y, 2 + len(data) - 1, " ")
								data = data[0:-1]
								send_win.erase()
								send_win.addstr(0, 0, "> " + data)
							else:
								data += chr(ch)
								send_win.addstr(send_y, send_x, data)

						send_win.refresh()

						try:
							readbuffer = server.recv(1024)
							splitted = readbuffer.split("\n")
							readbuffer = splitted.pop()
							y = handle(splitted, server, msg_win, y)
						except:
							pass
				except KeyboardInterrupt:
						running = False
						server.send("QUIT My IRC client just crashed :P\r\n")
						break

    server.close()
if(len(sys.argv) < 4):
    print("Usage: python irc_client.py <server> <nick> <channel>")
else:
    curses.wrapper(win)
    curses.endwin()
