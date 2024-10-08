# !/usr/bin/env python3
from io import BytesIO
from zipfile import ZipFile
import pathlib
from urllib.request import Request, urlopen
import os
import sys

download_asset_urls = [
	"https://casual-effects.com/g3d/data10/research/model/bunny/bunny.zip",
	"https://casual-effects.com/g3d/data10/common/model/crytek_sponza/sponza.zip",
	"https://casual-effects.com/g3d/data10/research/model/erato/erato.zip",
	"https://casual-effects.com/g3d/data10/research/model/rungholt/rungholt.zip",
	"https://casual-effects.com/g3d/data10/research/model/sibenik/sibenik.zip",
	"https://casual-effects.com/g3d/data10/research/model/sportsCar/sportsCar.zip",
	"https://casual-effects.com/g3d/data10/common/model/teapot/teapot.zip",
	"https://casual-effects.com/g3d/data10/research/model/dragon/dragon.zip",
	"https://casual-effects.com/g3d/data10/common/model/CornellBox/CornellBox.zip"]

output = sys.argv[1]
# output = "demo_asset/"

os.makedirs(output, exist_ok=True)

for url in download_asset_urls:
	req = Request(url)
	req.add_header(
		'user-agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/103.0.0.0 Safari/537.36')
	resp = urlopen(req)

	file = BytesIO(resp.read())

	with ZipFile(file) as zfile:
		# Extract subfolder
		directory_base = pathlib.Path(url).stem
		full_path = os.path.join(output, directory_base)
		os.makedirs(full_path, exist_ok=True)

		# Extract
		zfile.extractall(path=full_path)
