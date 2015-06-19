#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import re
import os
import sys

from cgrusequence import cgruSequence

import af

from optparse import OptionParser

ImgTypes = ['jpg','jpeg','dpx','cin','exr','tif','tiff','tga','png','psd']

Parser = OptionParser(
	usage="%prog [options] inarg\ntype \"%prog -h\" for help",
	version="%prog 1.0"
)

Parser.add_option('-t', '--type',       dest='type',       type  ='string',     default='jpg',  help='Image type')
Parser.add_option('-c', '--colorspace', dest='colorspace', type  ='string',     default='auto', help='Input images colorspace')
Parser.add_option('-r', '--resize',     dest='resize',     type  ='string',     default='',     help='Resize (1280x720)')
Parser.add_option('-q', '--quality',    dest='quality',    type  ='int',        default=75,     help='Quality')
Parser.add_option(      '--renumpad',   dest='renumpad',   type  ='int',        default=None,   help='Renumerate padding')
Parser.add_option('-o', '--output',     dest='output',     type  ='string',     default=None,   help='Output folder')
Parser.add_option('-A', '--afanasy',    dest='afanasy',    action='store_true', default=False,  help='Use Afanasy')
Parser.add_option(      '--afuser',     dest='afuser',     type  ='string',     default='',     help='Afanasy user')
Parser.add_option(      '--affpt',      dest='affpt',      type  ='int',        default='10',   help='Afanasy frames per task')
Parser.add_option(      '--afmax',      dest='afmax',      type  ='int',        default='-1',   help='Afanasy maximum running tasks')
Parser.add_option(      '--afmph',      dest='afmph',      type  ='int',        default='-1',   help='Afanasy max tasks per host')
Parser.add_option(      '--afmrt',      dest='afmrt',      type  ='int',        default='-1',   help='Afanasy max running time')
Parser.add_option(      '--afcap',      dest='afcap',      type  ='int',        default='-1',   help='Afanasy capacity')
Parser.add_option('-I', '--identify',   dest='identify',   action='store_true', default=False,  help='Identify image')
Parser.add_option('-J', '--json',       dest='json',       action='store_true', default=False,  help='Output JSON summary')
Parser.add_option('-V', '--verbose',    dest='verbose',    action='store_true', default=False,  help='Verbose mode')
Parser.add_option('-D', '--debug',      dest='debug',      action='store_true', default=False,  help='Debug mode')

Options, Args = Parser.parse_args()

OUT = dict()
OUT['convert'] = []


def errorExit(i_err):
	OUT['error'] = i_err
	print(json.dumps(OUT))
	sys.exit(1)


if len(Args) < 1:
	errorExit('Input is not specified.')

ResQual = ''
if Options.resize != '':
	ResQual += '.r%s' % Options.resize
if Options.type == 'jpg':
	ResQual += '.q%d' % Options.quality

Sequences = []
JobNames = []
MkDirs = []

for inarg in Args:
	inarg = os.path.normpath( inarg)
	files = []
	arg_is_file = False
	arg_is_folder = True
	if os.path.isfile(inarg):
		files = [inarg]
		arg_is_file = True
		arg_is_folder = False
	elif os.path.isdir(inarg):
		for afile in os.listdir(inarg):
			# check not hidden:
			if afile[0] == '.': continue

			# check extension for known image:
			name,ext = os.path.splitext(afile)
			if len(ext) < 1: continue
			ext = ext.lower()[1:]
			if not ext in ImgTypes: continue

			afile = os.path.join(inarg, afile)

			# check existance:
			if not os.path.isfile(afile): continue

			files.append(afile)

	else:
		errorExit('%s not found.' % inarg)

	outfolder = Options.output
	if outfolder is None:
		outfolder = inarg
	if arg_is_file:
		outfolder = os.path.dirname( outfolder)
	else:
		outfolder += '%s.%s' % ( ResQual, Options.type )

	seqs = cgruSequence(files, True)

	for seq in seqs:
		cmd = 'convert'

		if Options.afanasy:
			cmd += ' -identify -verbose'
		elif Options.identify:
			cmd += ' -identify'

		if seq['seq']:
			inseq = '%s%%0%dd%s' % (seq['prefix'],seq['padding'],seq['suffix'])
			if Options.afanasy:
				cmd += ' "%s[@FIRST@-@LAST@]"' % inseq
			else:
				cmd += ' "%s[%d-%d]"' % (inseq,seq['first'],seq['last'])
		else:
			inseq = seq['name']
			cmd += ' "%s"' % inseq
			cmd += ' -flatten' # To merge layers (tif,psd)

		# Process inarg colorspace:
		if Options.colorspace != 'auto':
			if Options.colorspace == 'extension':
				if imgtype == 'exr':
					cmd += ' -set colorspace RGB'
				elif imgtype == 'dpx':
					cmd += ' -set colorspace Log'
				elif imgtype == 'cin':
					cmd += ' -set colorspace Log'
				else:
					cmd += ' -set colorspace sRGB'
			else:
				cmd += ' -set colorspace ' + Options.colorspace

		# Resize sequence:
		if Options.resize != '':
			cmd += ' -resize %s' % Options.resize

		# Process ouput colorspace:
		out_ext = Options.type
		colorspace = ' -colorspace sRGB'
		if Options.type == 'jpg':
			cmd += ' -quality %d' % Options.quality
			cmd += ' -colorspace sRGB'
		elif Options.type == 'dpx':
			cmd += ' -depth 10'
			cmd += ' -colorspace Log'
		elif Options.type == 'tif8':
			out_ext = 'tif'
			cmd += ' -depth 8'
			cmd += ' -colorspace sRGB'
		elif Options.type == 'tif16':
			out_ext = 'tif'
			cmd += ' -depth 16'
			cmd += ' -colorspace sRGB'
		elif Options.type == 'exr':
			cmd += ' -colorspace RGB'

		outseq = inseq
		if seq['seq']:
			if Options.renumpad:
				outseq = '%s%%0%dd%s' % (seq['prefix'],Options.renumpad,seq['suffix'])
				if Options.afanasy:
					cmd += ' -scene @SCENE@'
				else:
					cmd += ' -scene 1'
			else:
				if Options.afanasy:
					cmd += ' -scene @SCENE@'
				else:
					cmd += ' -scene %d' % seq['first']

		outseq = os.path.join( outfolder, os.path.basename( outseq))

		if arg_is_folder:
			outseq,old_ext = os.path.splitext(outseq)
		else:
			outseq += ResQual
		outseq += '.' + out_ext

		cmd += ' "%s"' % outseq

		seq['cmd'] = cmd
		seq['cmd_name'] = os.path.basename( inseq)
		if seq['seq']:
			seq['cmd_name'] += ':%d-%d=%d' % (seq['first'],seq['last'],seq['count'])

	mkdir = None
	if arg_is_folder: mkdir = outfolder
	MkDirs.append(mkdir)
	JobNames.append(os.path.basename(inarg))

	Sequences.append(seqs)


for i in range(0, len(Sequences)):

	if MkDirs[i]:
		if Options.verbose:
			print('mkdir ' + MkDirs[i])
		if not Options.debug and not os.path.isdir(MkDirs[i]):
			try:
				os.makedirs(MkDirs[i])
			except:
				errorExit('Can`t create folder: ' + MkDirs[i])

	if Options.afanasy:
		job = af.Job('CVT ' + JobNames[i])

		if Options.afuser != '':
			job.setUserName(Options.afuser)
		if Options.afmax != -1:
			job.setMaxRunningTasks(Options.afmax)
		if Options.afmph != -1:
			job.setMaxRunTasksPerHost(Options.afmph)

	for j in range(0, len(Sequences[i])):
		if Options.verbose or Options.debug:
			print(Sequences[i][j]['cmd'])

		if Options.afanasy:
			block = af.Block(Sequences[i][j]['cmd_name'])
			job.blocks.append(block)

			if Sequences[i][j]['seq']:
				first = Sequences[i][j]['first']
				while first <= Sequences[i][j]['last']:
					last = first + Options.affpt
					if last > Sequences[i][j]['last']:
						last = Sequences[i][j]['last']
					scene = first
					if Options.renumpad:
						scene = first - Sequences[i][j]['first'] + 1
					
					task = af.Task( Sequences[i][j]['cmd_name'] + ('[%d-%d]' % (first,last)))
					cmd = Sequences[i][j]['cmd']
					cmd = cmd.replace('@FIRST@', str(first))
					cmd = cmd.replace('@LAST@',  str(last ))
					cmd = cmd.replace('@SCENE@', str(scene))
					task.setCommand( cmd)

					block.tasks.append( task)
					first += Options.affpt
			else:
				task = af.Task( Sequences[i][j]['cmd_name'])
				task.setCommand( Sequences[i][j]['cmd'])
				block.tasks.append( task)

			if Options.afcap != -1:
				block.setCapacity(Options.afcap)
			if Options.afmrt != -1:
				block.setTasksMaxRunTime(Options.afmrt)

		if not Options.afanasy and not Options.debug:
			os.system(Sequences[i][j]['cmd'])

	if Options.afanasy:
		if Options.verbose or Options.debug:
			job.output()
		if not Options.debug:
			job.send()

OUT['convert'] = Sequences
if Options.json:
	print(json.dumps(OUT))

