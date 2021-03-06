#include "convertgtp.h"

#include <qfile.h>
#include <qdatastream.h>

ConvertGtp::ConvertGtp(TabSong *song): ConvertBase(song)
{
	strongChecks = TRUE;
}

ConvertGtp::~ConvertGtp(){

}

QString ConvertGtp::readDelphiString()
{
	QString str;
	quint8 l;
	char *c;

	int maxl = readDelphiInteger();
	if (stream->device()->atEnd()) 
    return QString("readDelphiString: EOF");
	(*stream) >> l;

	if (maxl != l + 1)  
    return QString("readDelphiString: first word doesn't match second byte");

	c = (char *) malloc(l + 5);

	if (stream->device()->size() - stream->device()->pos() < l) {
		return QString("readDelphiString: not enough bytes to read %1 byte string").arg(l);
	}

	if (c) {
		stream->readRawData(c, l);
		c[l] = 0;
		str = QString::fromLocal8Bit(c);
		free(c);
	}

	return str;
}

QString ConvertGtp::readPascalString(int maxlen)
{
	QString str;
	quint8 l;
	char *c;

	(*stream) >> l;

	c = (char *) malloc(l + 5);

	if (c) {
		stream->readRawData(c, l);
		c[l] = 0;
		str = QString::fromLocal8Bit(c);
		free(c);
	}

	// Skip garbage after pascal string end
	c = (char *) malloc(maxlen + 5);
	stream->readRawData(c, maxlen - l);
	free(c);

	return str;
}

QString ConvertGtp::readWordPascalString()
{
	QString str;
	char *c;

	int l = readDelphiInteger();

	c = (char *) malloc(l + 5);

	if (c) {
		stream->readRawData(c, l);
		c[l] = 0;
		str = QString::fromLocal8Bit(c);
		free(c);
	}

	return str;
}

int ConvertGtp::readDelphiInteger()
{
	quint8 x;
	int r;
	if (stream->device()->atEnd())  throw QString("readDelphiInteger: EOF");
	(*stream) >> x; r = x;
	if (stream->device()->atEnd())  throw QString("readDelphiInteger: EOF");
	(*stream) >> x; r += x << 8;
	if (stream->device()->atEnd())  throw QString("readDelphiInteger: EOF");
	(*stream) >> x; r += x << 16;
	if (stream->device()->atEnd())  throw QString("readDelphiInteger: EOF");
	(*stream) >> x; r += x << 24;
	return r;
}

void ConvertGtp::readChromaticGraph()
{
	quint8 num;
	int n;

	// GREYFIX: currently just skips over chromatic graph
	(*stream) >> num;                        // icon
	readDelphiInteger();                     // shown amplitude
	n = readDelphiInteger();                 // number of points
	for (int i = 0; i < n; i++) {
		readDelphiInteger();                 // time
		readDelphiInteger();                 // pitch
		(*stream) >> num;                    // vibrato
	}
}

void ConvertGtp::readChord()
{
	int x1, x2, x3, x4;
	quint8 num;
	QString text;
	char garbage[50];
	// GREYFIX: currently just skips over chord diagram

	// GREYFIX: chord diagram
	x1 = readDelphiInteger();
	//if (x1 != 257)
	//	qDebug() << "Chord INT1=" << x1 << ", not 257\n";
	x2 = readDelphiInteger();
	//if (x2 != 0)
	//	qDebug() << "Chord INT2=" << x2 << ", not 0\n";
	x3 = readDelphiInteger();
	//qDebug() << "Chord INT3: " << x3 << "\n"; // FF FF FF FF if there is diagram
	x4 = readDelphiInteger();
	/*if (x4 != 0)
		qDebug() << "Chord INT4=" << x4 << ", not 0\n";*/
	(*stream) >> num;
	/*if (num != 0)
		qDebug() << "Chord BYTE5=" << (int) num << ", not 0\n";*/
	text = readPascalString(25);
	//qDebug() << "Chord diagram: " << text << "\n";
	
	// Chord diagram parameters - for every string
	for (int i = 0; i < STRING_MAX_NUMBER; i++) {
		x1 = readDelphiInteger();
		//qDebug() << x1 << "\n";
	}
	
	// Unknown bytes
	stream->readRawData(garbage, 36);

	//qDebug() << "after chord, position: " << stream->device()->pos() << "\n";
}

void ConvertGtp::readSignature()
{
	currentStage = QString("readSignature");

	QString s = readPascalString(30);        // Format string
	//qDebug() << "GTP format: \"" << s << "\"\n";

	// Parse version string
	if (s == "FICHIER GUITARE PRO v1") {
		versionMajor = 1; versionMinor = 0;
	} else if (s == "FICHIER GUITARE PRO v1.01") {
		versionMajor = 1; versionMinor = 1;
	} else if (s == "FICHIER GUITARE PRO v1.02") {
		versionMajor = 1; versionMinor = 2;
	} else if (s == "FICHIER GUITARE PRO v1.03") {
		versionMajor = 1; versionMinor = 3;
	} else if (s == "FICHIER GUITARE PRO v1.04") {
		versionMajor = 1; versionMinor = 4;
	} else if (s == "FICHIER GUITAR PRO v2.20") {
		versionMajor = 2; versionMinor = 20;
	} else if (s == "FICHIER GUITAR PRO v2.21") {
		versionMajor = 2; versionMinor = 21;
	} else if (s == "FICHIER GUITAR PRO v3.00") {
		versionMajor = 3; versionMinor = 0;
	} else if (s == "FICHIER GUITAR PRO v4.00") {
		versionMajor = 4; versionMinor = 0;
	} else if (s == "FICHIER GUITAR PRO v4.06") {
		versionMajor = 4; versionMinor = 6;
	} else if (s == "FICHIER GUITAR PRO L4.06") {
		versionMajor = 4; versionMinor = 6;
	} else {
		qDebug() << QString("Invalid file format: \"%1\"").arg(s);
	}
}

void ConvertGtp::readSongAttributes()
{
	QString s;
	char garbage[10];

	quint8 num;

	currentStage = QString("readSongAttributes: song->info");

	song->info["TITLE"] = readDelphiString();
	song->info["SUBTITLE"] = readDelphiString();
	song->info["ARTIST"] = readDelphiString();
	song->info["ALBUM"] = readDelphiString();
	song->info["COMPOSER"] = readDelphiString();
	song->info["COPYRIGHT"] = readDelphiString();
	song->info["TRANSCRIBER"] = readDelphiString();
	song->info["INSTRUCTIONS"] = readDelphiString();

	// Notice lines
	currentStage = QString("readSongAttributes: notice lines");
	song->info["COMMENTS"] = "";
	int n = readDelphiInteger();
	for (int i = 0; i < n; i++)
		song->info["COMMENTS"] += readDelphiString() + "\n";

	currentStage = QString("readSongAttributes: shuffle rhythm feel");
	(*stream) >> num;                        // GREYFIX: Shuffle rhythm feel

	currentStage = QString("readSongAttributes: lyrics");
	// Lyrics
	readDelphiInteger();                     // GREYFIX: Lyric track number start
	for (int i = 0; i < LYRIC_LINES_MAX_NUMBER; i++) {
		readDelphiInteger();                 // GREYFIX: Start from bar
		readWordPascalString();              // GREYFIX: Lyric line
	}

	currentStage = QString("readSongAttributes: tempo");
	song->tempo = readDelphiInteger();       // Tempo

	stream->readRawData(garbage, 5);        // Mysterious bytes
}

void ConvertGtp::readTrackDefaults()
{
	quint8 num;
	currentStage = QString("readTrackDefaults");

	for (int i = 0; i < TRACK_MAX_NUMBER * 2; i++) {
		trackPatch[i] = readDelphiInteger(); // MIDI Patch
		(*stream) >> num;                    // GREYFIX: volume
		(*stream) >> num;                    // GREYFIX: pan
		(*stream) >> num;                    // GREYFIX: chorus
		(*stream) >> num;                    // GREYFIX: reverb
		(*stream) >> num;                    // GREYFIX: phase
		(*stream) >> num;                    // GREYFIX: tremolo

		(*stream) >> num;                    // 2 byte padding: must be 00 00
		if (num != 0)  throw QString("1 of 2 byte padding: there is %1, must be 0").arg(num);
		(*stream) >> num;
		if (num != 0)  throw QString("2 of 2 byte padding: there is %1, must be 0").arg(num);
	}
}

void ConvertGtp::readBarProperties()
{
	quint8 bar_bitmask, num;

	int time1 = 4;
	int time2 = 4;
	int keysig = 0;

	bars.resize(numBars);

	currentStage = QString("readBarProperties");
	//qDebug() << "readBarProperties(): start\n";

	for (int i = 0; i < numBars; i++) {
		(*stream) >> bar_bitmask;                    // bar property bitmask
		//if (bar_bitmask != 0)
		//	qDebug() << "BAR #" << i << " - flags " << (int) bar_bitmask << "\n";
		// GREYFIX: new_time_numerator
		if (bar_bitmask & 0x01) {
			(*stream) >> num;
			time1 = num;
			//qDebug() << "new time1 signature: " << time1 << ":" << time2 << "\n";
		}
		// GREYFIX: new_time_denominator
		if (bar_bitmask & 0x02) {
			(*stream) >> num;
			time2 = num;
			//qDebug() << "new time2 signature: " << time1 << ":" << time2 << "\n";
		}
		// GREYFIX: begin repeat
		//if (bar_bitmask & 0x04) {
		//	qDebug() << "begin repeat\n";
		//}
		// GREYFIX: number_of_repeats
		if (bar_bitmask & 0x08) {
			(*stream) >> num;
			//qDebug() << "end repeat " << (int) num << "x\n";
		}
		// GREYFIX: alternative_ending_to
		if (bar_bitmask & 0x10) {
			(*stream) >> num;
			//qDebug() << "alternative ending to " << (int) num << "\n";
		}
		// GREYFIX: new section
		if (bar_bitmask & 0x20) {
			QString text = readDelphiString();
			readDelphiInteger(); // color?
			//qDebug() << "new section: " << text << "\n";
		}
		if (bar_bitmask & 0x40) {
			(*stream) >> num;                // GREYFIX: alterations_number
			keysig = num;
			(*stream) >> num;                // GREYFIX: minor
			//qDebug() << "new key signature (" << keysig << ", " << num << ")\n";
		}
		// GREYFIX: double bar
		//if (bar_bitmask & 0x80) {
		//	qDebug() << "double bar\n";
		//}

		bars[i].time1 = time1;
		bars[i].time2 = time1;
		bars[i].keysig = keysig;
	}
	//qDebug() << "readBarProperties(): end\n";
}

void ConvertGtp::readTrackProperties()
{
	quint8 num;
	int strings, midiChannel2, capo, color;

	currentStage = QString("readTrackProperties");
	//qDebug() << "readTrackProperties(): start\n";

	for (int i = 0; i < numTracks; i++) {
		(*stream) >> num;                    // GREYFIX: simulations bitmask
		//qDebug() << "Simulations: " << num << "\n";

		song->t.append(new TabTrack(TabTrack::FretTab, 0, 0, 0, 0, 6, 24));
    TabTrack *trk = song->t.at(i); //-------------------------------------------------------------------

		trk->name = readPascalString(40);    // Track name
		//qDebug() << "Track: " << trk->name << "\n";

		// Tuning information

		//qDebug() << "pos: " << stream->device()->pos() << "\n";

		strings = readDelphiInteger();
		if (strings <= 0 || strings > STRING_MAX_NUMBER)  throw QString("Track %1: insane # of strings (%2)\n").arg(i).arg(strings);
		trk->string = strings;

		// Parse [0..string-1] with real string tune data in reverse order
		for (int j = trk->string - 1; j >= 0; j--) {
			trk->tune[j] = readDelphiInteger();
			if (trk->tune[j] > 127)  throw QString("Track %1: insane tuning on string %2 = %3\n").arg(i).arg(j).arg(trk->tune[j]);
		}

		// Throw out the other useless garbage in [string..MAX-1] range
		for (int j = trk->string; j < STRING_MAX_NUMBER; j++)
			readDelphiInteger();

		// GREYFIX: auto flag here?

		readDelphiInteger();                 // GREYFIX: MIDI port
		trk->channel = readDelphiInteger();  // MIDI channel 1
		midiChannel2 = readDelphiInteger();  // GREYFIX: MIDI channel 2
		trk->frets = readDelphiInteger();    // Frets
		capo = readDelphiInteger();          // GREYFIX: Capo
		color = readDelphiInteger();         // GREYFIX: Color

		//qDebug() <<
		//	"MIDI #" << trk->channel << "/" << midiChannel2 << ", " <<
		//	trk->string << " strings, " <<
		//	trk->frets << " frets, capo " <<
		//	capo << "\n";

		if (trk->frets <= 0 || (strongChecks && trk->frets > 100))  throw QString("Track %1: insane number of frets (%2)\n").arg(i).arg(trk->frets);
		if (trk->channel < 0 || trk->channel > 16)  throw QString("Track %1: insane MIDI channel 1 (%2)\n").arg(i).arg(trk->channel);
		if (midiChannel2 < 0 || midiChannel2 > 16)  throw QString("Track %1: insane MIDI channel 2 (%2)\n").arg(i).arg(midiChannel2);

		// Fill remembered values from defaults
		trk->patch = trackPatch[i];
	}
	//qDebug() << "readTrackProperties(): end\n";
}

void ConvertGtp::readTabs()
{
	quint8 beat_bitmask, stroke_bitmask1, stroke_bitmask2, strings, num;
	qint8 length, volume, pan, chorus, reverb, phase, tremolo;
	int x;

	currentStage = QString("readTabs");

	//TabTrack *trk = song->t.first();
  TabTrack *trk;
	for (int tr = 0; tr < numTracks; tr++) {
    trk = song->t.at(tr);
    trk->b.resize(numBars);
		trk->c.resize(0);
		//trk = song->t.next();
	}

	for (int j = 0; j < numBars; j++) {
		//TabTrack *trk = song->t.first();
		TabTrack *trk;
    for (int tr = 0; tr < numTracks; tr++) {
      trk = song->t.at(tr);
			int numBeats = readDelphiInteger();
			//qDebug() << "TRACK " << tr << ", BAR " << j << ", numBeats " << numBeats << " (position: " << stream->device()->pos() << ")\n";

			if (numBeats < 0 || (strongChecks && numBeats > 128))  throw QString("Track %1, bar %2, insane number of beats: %3").arg(tr).arg(j).arg(numBeats);

			x = trk->c.size();
			trk->c.resize(trk->c.size() + numBeats);
			trk->b[j].time1 = bars[j].time1;
			trk->b[j].time2 = bars[j].time2;
			trk->b[j].keysig = bars[j].keysig;
			trk->b[j].start = x;

			for (int k = 0; k < numBeats; k++) {
				trk->c[x].flags = 0;

				(*stream) >> beat_bitmask;   // beat bitmask
				
				if (beat_bitmask & 0x01)     // dotted column
					trk->c[x].flags |= FLAG_DOT;

				if (beat_bitmask & 0x40) {
					(*stream) >> num;        // GREYFIX: pause_kind
				}

				// Guitar Pro 4 beat lengths are as following:
				// -2 = 1    => 480     3-l = 5  2^(3-l)*15
				// -1 = 1/2  => 240           4
				//  0 = 1/4  => 120           3
				//  1 = 1/8  => 60            2
				//  2 = 1/16 => 30 ... etc    1
				//  3 = 1/32 => 15            0

				(*stream) >> length;            // length
				//qDebug() << "beat_bitmask: " << (int) beat_bitmask << "; length: " << length << "\n";

				trk->c[x].l = (1 << (3 - length)) * 15;

				if (beat_bitmask & 0x20) {
					int tuple = readDelphiInteger();
					//qDebug() << "Tuple: " << tuple << "\n"; // GREYFIX: t for tuples
					if (!(tuple == 3 || (tuple >= 5 && tuple <= 7) || (tuple >= 9 && tuple <= 13)))  throw QString("Insane tuple t: %1").arg(tuple);
				}
				
				if (beat_bitmask & 0x02)     // Chord diagram
					readChord();

				if (beat_bitmask & 0x04) {
          readDelphiString();
					//qDebug() << "Text: " << readDelphiString() << "\n"; // GREYFIX: text with a beat
				}
				
				// GREYFIX: stroke bitmasks
				if (beat_bitmask & 0x08) {
					(*stream) >> stroke_bitmask1;
					(*stream) >> stroke_bitmask2;
					if (stroke_bitmask1 & 0x20)
						(*stream) >> num;      // GREYFIX: string torture
					if (stroke_bitmask2 & 0x04)
						readChromaticGraph();  // GREYFIX: tremolo graph
					if (stroke_bitmask1 & 0x40) {
						(*stream) >> num;      // GREYFIX: down stroke length
						(*stream) >> num;      // GREYFIX: up stroke length
					}
					if (stroke_bitmask2 & 0x02) {
						(*stream) >> num;      // GREYFIX: stroke pick direction
					}
				}
				
				if (beat_bitmask & 0x10) {     // mixer variations
					(*stream) >> num;          // GREYFIX: new MIDI patch
					(*stream) >> volume;       // GREYFIX: new
					(*stream) >> pan;          // GREYFIX: new
					(*stream) >> chorus;       // GREYFIX: new
					(*stream) >> reverb;       // GREYFIX: new
					(*stream) >> phase;        // GREYFIX: new
					(*stream) >> tremolo;      // GREYFIX: new
					int tempo = readDelphiInteger(); // GREYFIX: new tempo

					// GREYFIX: transitions
					if (volume != -1)   (*stream) >> num;
					if (pan != -1)      (*stream) >> num;
					if (chorus != -1)   (*stream) >> num;
					if (reverb != -1)   (*stream) >> num;
					if (tremolo != -1)  (*stream) >> num;
					if (tempo != -1)    (*stream) >> num;

					(*stream) >> num;          // padding
				}
				
				(*stream) >> strings;          // used strings mask
				
				for (int y = STRING_MAX_NUMBER - 1; y >= 0; y--) {
					trk->c[x].e[y] = 0;
					trk->c[x].a[y] = NULL_NOTE;
					if (strings & (1 << (y + STRING_MAX_NUMBER - trk->string)))
						readNote(trk, x, y);
				}
				
				x++;
			}
			//trk = song->t.next();
		}
	}
}

void ConvertGtp::readNote(TabTrack *trk, int x, int y)
{
	quint8 note_bitmask, variant, num, mod_mask1, mod_mask2;

	(*stream) >> note_bitmask;               // note bitmask
	(*stream) >> variant;                    // variant

	if (note_bitmask & 0x01) {               // GREYFIX: note != beat
		(*stream) >> num;                    // length
		(*stream) >> num;                    // t
	}

	if (note_bitmask & 0x02) {};             // GREYFIX: note is dotted

	if (note_bitmask & 0x10) {               // GREYFIX: dynamic
		(*stream) >> num;
	}

	(*stream) >> num;                        // fret number
	trk->c[x].a[y] = num;

	if (variant == 2) {                      // link with previous beat
		trk->c[x].flags |= FLAG_ARC;
		for (uint i = 0; i < MAX_STRINGS; i++) {
			trk->c[x].a[i] = NULL_NOTE;
			trk->c[x].e[i] = 0;
		}
	}

	if (variant == 3)                        // dead notes
		trk->c[x].a[y] = DEAD_NOTE;

	if (note_bitmask & 0x80) {               // GREYFIX: fingering
		(*stream) >> num;
		(*stream) >> num;
	}

	if (note_bitmask & 0x08) {
		(*stream) >> mod_mask1;
		(*stream) >> mod_mask2;
		if (mod_mask1 & 0x01) {
			readChromaticGraph();            // GREYFIX: bend graph
		}
		if (mod_mask1 & 0x02)                // hammer on / pull off
			trk->c[x].e[y] |= EFFECT_LEGATO;
		if (mod_mask1 & 0x08)                // let ring
			trk->c[x].e[y] |= EFFECT_LETRING;
		if (mod_mask1 & 0x10) {              // GREYFIX: graces
			(*stream) >> num;                // GREYFIX: grace fret
			(*stream) >> num;                // GREYFIX: grace dynamic
			(*stream) >> num;                // GREYFIX: grace transition
			(*stream) >> num;                // GREYFIX: grace length
		}
		if (mod_mask2 & 0x01)                // staccato - we do palm mute
			trk->c[x].flags |= FLAG_PM;
		if (mod_mask2 & 0x02)                // palm mute - we mute the whole column
			trk->c[x].flags |= FLAG_PM;
		if (mod_mask2 & 0x04) {              // GREYFIX: tremolo
			(*stream) >> num;                // GREYFIX: tremolo picking length
		}
		if (mod_mask2 & 0x08) {              // slide
			trk->c[x].e[y] |= EFFECT_SLIDE;
			(*stream) >> num;                // GREYFIX: slide kind
		}
		if (mod_mask2 & 0x10) {              // GREYFIX: harmonic
			(*stream) >> num;                // GREYFIX: harmonic kind
		}
		if (mod_mask2 & 0x20) {              // GREYFIX: trill
			(*stream) >> num;                // GREYFIX: trill fret
			(*stream) >> num;                // GREYFIX: trill length
		}
	}
}

bool ConvertGtp::load(QString fileName)
{
	QFile f(fileName);
	if (!f.open(QIODevice::ReadOnly))
		qDebug()<< QString("Unable to open file for reading");

	QDataStream s(&f);
	stream = &s;

	try {
//		song = new TabSong();

		readSignature();
		song->t.clear();
		readSongAttributes();
	 	readTrackDefaults();

	 	numBars = readDelphiInteger();           // Number of bars
	 	numTracks = readDelphiInteger();         // Number of tracks

		//qDebug() << "Bars: " << numBars << "\n";
		//qDebug() << "Tracks: " << numTracks << "\n";

	 	readBarProperties();
	 	readTrackProperties();
	 	readTabs();

		currentStage = QString("Exit code");
		if (!f.atEnd()) {
			int ex = readDelphiInteger();            // Exit code: 00 00 00 00
			if (ex != 0)
				qDebug() << "File not ended with 00 00 00 00\n";
			if (!f.atEnd())
				qDebug() << "File not ended - there's more data!\n";
		}
	} catch (QString msg) {
			qDebug()<< QString("Guitar Pro import error:") + QString("\n") +
			msg + QString("\n") +
			  QString("Stage: %1").arg(currentStage) + QString("\n") +
			QString("File position: %1/%2").arg(f.pos()).arg(f.size());
	}

	f.close();

	return song;
}

bool ConvertGtp::save(QString)
{
	qDebug()<< QString("Not implemented");
  return false;
}
