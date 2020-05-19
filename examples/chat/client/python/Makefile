all:
	protoc --python_out=. -I ../.. chat.proto
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_py_source ../../chat.proto
	PYTHONPATH=../../../.. python3 main.py Python
